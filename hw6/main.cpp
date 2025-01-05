// TODO: Pruning
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::random_device rndd;
std::default_random_engine rnde(rndd());

using CSVLine = std::vector<std::string>;
using CSV = std::vector<CSVLine>;

class CSVReader {
public:
  static CSV readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file.");
    }

    CSV lines;
    std::string line;
    while (std::getline(file, line)) {
      std::istringstream iss(line);
      CSVLine cols;
      std::string token;

      while (std::getline(iss, token, ',')) {
        cols.push_back(token);
      }

      lines.push_back(cols);
    }
    file.close();

    return lines;
  }
};

using Label = std::string;
using Attribute = std::string;

struct DatasetRow {
  Label label;
  std::vector<Attribute> attributes;
  DatasetRow(const Label& label,
             const std::vector<Attribute>& attributes)
      : label(label), attributes(attributes) {}

  static const Label& labelSelector(const DatasetRow& instance) {
    return instance.label;
  }
};

class DatasetReader {
public:
  static std::vector<DatasetRow> readCSV(const CSV& csv, int mode) {
    std::vector<DatasetRow> dataset;
    for (const auto& row : csv) {
      if (!row.empty()) {
        Label label = row[0];
        std::vector<Attribute> attributes(row.begin() + 1, row.end());
        dataset.emplace_back(label, attributes);
      }
    }
    return dataset;
  }
};

class ID3DecisionTree {
private:
  struct Node {
    std::optional<size_t> attrIndex;
    std::optional<Attribute> attrValue;
    std::unordered_map<Label, size_t> results;
    std::vector<std::shared_ptr<Node>> children;

    Node(const std::unordered_map<Label, size_t>& results)
      : results(results) {
    }

    Node(const size_t attrIndex)
      : attrIndex(attrIndex) {
    }
  };

  std::shared_ptr<Node> root;

public:
  void train(const std::vector<DatasetRow>& data) {
    root = nullptr;
    if (data.empty()) {
      return;
    }

    std::vector<size_t> attrIndices(data[0].attributes.size());
    std::iota(attrIndices.begin(), attrIndices.end(), 0);
    root = buildTree(data, attrIndices);
  }

  Label predict(const std::vector<Attribute>& attributes) const {
    if (!root) {
      throw std::runtime_error("Tree is not trained.");
    }

    auto node = root;
    while (node->results.empty()) {
      Attribute attrValue = attributes[node->attrIndex.value()];
      auto it = std::find_if(node->children.begin(), node->children.end(), [&](const auto& n) { return n->attrValue == attrValue; });
      if (it == node->children.end()) {
        throw std::runtime_error("No matching branch.");
      }
      node = *it;
    }

    return std::max_element(
      node->results.begin(),
      node->results.end(),
      [](const auto& a, const auto& b) { return a.second < b.second; }
    )->first;
  }

private:
  static std::shared_ptr<Node> buildTree(const std::vector<DatasetRow>& data, const std::vector<size_t>& attrIndices) {
    std::vector<Label> labels;
    std::transform(data.begin(), data.end(), std::back_inserter(labels), DatasetRow::labelSelector);
    std::unordered_set labelsSet(labels.begin(), labels.end());

    std::optional<size_t> bestAttrIndex;
    if (labelsSet.size() == 1 || attrIndices.empty() || !(bestAttrIndex = findeBestAttrIndex(data, attrIndices))) {
      std::unordered_map<Label, size_t> results;
      for (const Label& label : labelsSet) {
        results[label] = std::count(labels.begin(), labels.end(), label);
      }
      return std::make_shared<Node>(results);
    }

    auto root = std::make_shared<Node>(bestAttrIndex.value());
    auto splits = splitData(data, bestAttrIndex.value());
    for (const auto& [value, subset] : splits) {
      std::vector<size_t> attrIndicesNew;
      std::copy_if(
        attrIndices.begin(),
        attrIndices.end(),
        std::back_inserter(attrIndicesNew),
        [&](const auto& x){ return x != bestAttrIndex; }
      );

      auto child = buildTree(data, attrIndicesNew);
      child->attrValue = value;

      root->children.push_back(child);
    }

    return root;
  }

  static std::optional<size_t> findeBestAttrIndex(const std::vector<DatasetRow>& data, const std::vector<size_t>& attrIndices) {
    std::optional<size_t> bestAttrIndex;
    double bestGain = 0.0;

    for (const size_t attrIndex : attrIndices) {
      double gain = calculateGain(data, attrIndex);
      if (gain > bestGain) {
        bestGain = gain;
        bestAttrIndex = attrIndex;
      }
    }

    return bestAttrIndex;
  }

  static double calculateGain(const std::vector<DatasetRow>& data, const size_t attrIndex) {
    double base_entropy = calculateEntropy(data);
    size_t total_samples = data.size();

    auto splits = splitData(data, attrIndex);
    double weighted_entropy = 0;
    for (const auto& [_, subset] : splits) {
      double prob = (double)subset.size() / total_samples;
      weighted_entropy += prob * calculateEntropy(subset);
    }

    return base_entropy - weighted_entropy;
  }

  static double calculateEntropy(const std::vector<DatasetRow>& data) {
    size_t total = data.size();
    std::unordered_map<Label, size_t> labelCounts;

    for (const auto& row : data) {
      labelCounts[row.label] += 1;
    }

    double entropy = 0;
    for (const auto& [_, count] : labelCounts) {
      double prob = (double)count / total;
      entropy -= prob * std::log2(prob);
    }
    return entropy;
  }

  static std::unordered_map<Attribute, std::vector<DatasetRow>> splitData(const std::vector<DatasetRow>& data, const size_t bestAttrIndex) {
    std::unordered_map<Attribute, std::vector<DatasetRow>> splits;
    for (const DatasetRow& row : data) {
      Attribute value = row.attributes[bestAttrIndex];
      splits[value].push_back(row);
    }
    return splits;
  }
};

template <typename T, typename S>
static void splitTrainTest(const std::vector<T>& dataset, const std::function<S(T)>& splitBy, std::vector<T>& train, std::vector<T>& test, const double ratio) {
  std::unordered_map<S, std::vector<T>> split;
  for (const auto& entry : dataset) {
    split[splitBy(entry)].push_back(entry);
  }

  train.clear();
  test.clear();
  for (const auto& splitVal : split) {
    std::vector<T> subdataset = splitVal.second;
    size_t trainSize = ratio * subdataset.size();

    std::shuffle(subdataset.begin(), subdataset.end(), rnde);

    std::vector<T> trainParty(subdataset.begin(), subdataset.begin() + trainSize);
    std::vector<T> testParty(subdataset.begin() + trainSize, subdataset.end());

    train.insert(train.end(), trainParty.begin(), trainParty.end());
    test.insert(test.end(), testParty.begin(), testParty.end());
  }

  std::shuffle(train.begin(), train.end(), rnde);
  std::shuffle(test.begin(), test.end(), rnde);
}

double calculateAccuracy(const std::vector<DatasetRow>& train, const std::vector<DatasetRow>& test) {
  ID3DecisionTree tree;
  tree.train(train);

  size_t predictions = 0;
  for (const DatasetRow& row : test) {
    if (tree.predict(row.attributes) == row.label) {
      predictions++;
    }
  }

  return (double)(predictions) / test.size();
}

std::vector<double> calculateKFoldAccuracy(const std::vector<DatasetRow>& dataset, const size_t K, double& mean, double& stdev) {
  const size_t testSize = dataset.size() / K;

  std::vector<double> accuracies;
  for (size_t i = 0; i < K; i++) {
    std::vector<DatasetRow> test;
    std::vector<DatasetRow> train;
    size_t testStart = i * testSize;

    train.insert(train.end(), dataset.begin(), dataset.begin() + testStart);
    test.insert(test.end(), dataset.begin() + testStart, dataset.begin() + testStart + testSize);
    train.insert(train.end(), dataset.begin() + testStart + testSize, dataset.end());

    double accuracy = calculateAccuracy(train, test);
    accuracies.push_back(accuracy);
  }

  double sum = std::accumulate(accuracies.begin(), accuracies.end(), 0.0);
  mean = sum / accuracies.size();

  double sq_sum = 0.0;
  for (double acc : accuracies) {
    sq_sum += std::pow(acc - mean, 2);
  }
  stdev = std::sqrt(sq_sum / accuracies.size());

  return accuracies;
}

void solve(const std::vector<DatasetRow> dataset) {
  std::vector<DatasetRow> train, test;
  splitTrainTest<DatasetRow, Label>(dataset, DatasetRow::labelSelector, train, test, 0.8);

  double trainAccuracy = calculateAccuracy(train, train);
  std::cout << "1. Train Set Accuracy:" << std::endl
            << "   Accuracy: " << trainAccuracy * 100 << "%" << std::endl;

  int K = 10;
  double mean, stdev;
  std::vector<double> accuracies = calculateKFoldAccuracy(train, K, mean, stdev);
  std::cout << K << "-Fold Cross-Validation Results:" << std::endl;
  for (size_t i = 0; i < accuracies.size(); i++) {
    std::cout << "    Accuracy Fold " << (i+1) << ": "<< accuracies[i] * 100 << "%"  << std::endl;
  }
  std::cout << std::endl
            << "    Average Accuracy: "<< mean * 100 << "%" << std::endl
            << "    Standard Deviation: "<< stdev * 100 << "%" << std::endl;

  double testAccuracy = calculateAccuracy(train, test);
  std::cout << "2. Test Set Accuracy:" << std::endl
            << "   Accuracy: " << testAccuracy * 100 << "%" << std::endl;
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 2) {
      throw std::runtime_error("Dataset file is required.");
    }

    const std::string filename = argv[1];
    const int mode = argc >= 3 ? std::atoi(argv[2]) : 0;

    const CSV csv = CSVReader::readFile(filename);
    const std::vector<DatasetRow> dataset = DatasetReader::readCSV(csv, mode);
    solve(dataset);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
