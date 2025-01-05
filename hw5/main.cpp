#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
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


enum class Party {
  D,
  R,
};


enum class Attribute {
  UNK,
  NAY,
  YAY,
};


struct Person {
  Party party;
  std::vector<Attribute> attributes;
  Person(const Party& party, const std::vector<Attribute>& attributes)
    : party(party), attributes(attributes)
  {}
};


class DatasetReader {
public:
  static std::vector<Person> readCSV(const CSV& csv, int mode) {
    std::unordered_map<Party, std::vector<int>> partyVotes;

    for (const CSVLine& line : csv) {
      const Party party = parseParty(line[0]);
      const std::vector<Attribute> attrs = parseAttributes(line);
      readPartyVotes(attrs, partyVotes[party]);
    }

    std::vector<Person> dataset;
    for (const CSVLine& line : csv) {
      const Party party = parseParty(line[0]);
      std::vector<Attribute> attrs = parseAttributes(line);
      if (mode == 1) {
        transformAttributes(attrs, partyVotes[party]);
      }
      dataset.emplace_back(party, attrs);
    }

    return dataset;
  }

private:
  static Party parseParty(const std::string& str) {
    if (str == "democrat") return Party::D;
    if (str == "republican") return Party::R;
    throw std::invalid_argument("Unknown value.");
  }

  static Attribute parseAttribute(const std::string& str) {
    if (str == "y") return Attribute::YAY;
    if (str == "n") return Attribute::NAY;
    if (str == "?") return Attribute::UNK;
    throw std::invalid_argument("Unknown value.");
  }

  static std::vector<Attribute> parseAttributes(const CSVLine& line) {
    std::vector<Attribute> attributes;
    for (size_t i = 1; i < line.size(); i++) {
      attributes.push_back(parseAttribute(line[i]));
    }
    return attributes;
  }

  static void readPartyVotes(const std::vector<Attribute>& attributes, std::vector<int>& partyVotes) {
    if (attributes.size() != partyVotes.size()) {
      if (partyVotes.size() != 0) {
        throw std::runtime_error("Inconsistent attribute count.");
      }
      partyVotes.resize(attributes.size());
    }

    for (size_t i = 0; i < attributes.size(); i++) {
      if (attributes[i] == Attribute::YAY) {
        partyVotes[i]++;
      } else if (attributes[i] == Attribute::NAY) {
        partyVotes[i]--;
      }
    }
  }

  static void transformAttributes(std::vector<Attribute>& attributes, const std::vector<int>& partyVotes) {
    for (size_t i = 0; i < attributes.size(); i++) {
      if (attributes[i] == Attribute::YAY || (attributes[i] == Attribute::UNK && partyVotes[i] > 0)) {
        attributes[i] = Attribute::YAY;
      } else {
        attributes[i] = Attribute::NAY;
      }
    }
  }
};


class NaiveBayesClassifier {
private:
  const std::vector<Person> dataset;
  std::unordered_map<Party, std::unordered_map<Attribute, std::vector<size_t>>> map;

public:
  NaiveBayesClassifier(const std::vector<Person>& dataset) :
    dataset(dataset)
  {}

  void train() {
    map.clear();

    for (const Person& person : dataset) {
      auto& attrMap = map[person.party];
      for (size_t i = 0; i < person.attributes.size(); i++) {
        attrMap.try_emplace(person.attributes[i], person.attributes.size(), 0);
        attrMap[person.attributes[i]][i]++;
      }
    }
  }

  bool predict(const Person& person) const {
    Party predictedParty = std::max({Party::D, Party::R}, [&](const Party a, const Party b) {
      return probability(person.attributes, a) < probability(person.attributes, b);
    });
    return person.party == predictedParty;
  }

private:
  double probability(const std::vector<Attribute>& attributes, const Party& party) const {
    const auto& attrMap = map.at(party);
    const int partyCount = countByParty(party);
    const int lambda = 1;
    const int k = 2;

    double probability = 0.0;
    for (size_t i = 0; i < attributes.size(); i++) {
      size_t matchCount = attrMap.at(attributes[i])[i];
      probability += std::log((double)(matchCount + lambda) / (partyCount + (k * lambda)));
    }
    probability += std::log((double)(partyCount + lambda) / (dataset.size() + (k * lambda)));
    return probability;
  }

  size_t countByParty(const Party party) const {
    return std::count_if(dataset.begin(), dataset.end(), [&](const auto& p){ return p.party == party; });
  }
};

static void splitTrainTest(const std::vector<Person>& dataset, std::vector<Person>& train, std::vector<Person>& test, const double ratio) {
  std::unordered_map<Party, std::vector<Person>> partySplit;
  for (const auto& p : dataset) {
    partySplit[p.party].push_back(p);
  }

  train.clear();
  test.clear();
  for (const auto& party : partySplit) {
    std::vector<Person> datasetParty = party.second;
    size_t trainSize = ratio * datasetParty.size();

    std::shuffle(datasetParty.begin(), datasetParty.end(), rnde);

    std::vector<Person> trainParty(datasetParty.begin(), datasetParty.begin() + trainSize);
    std::vector<Person> testParty(datasetParty.begin() + trainSize, datasetParty.end());
    
    train.insert(train.end(), trainParty.begin(), trainParty.end());
    test.insert(test.end(), testParty.begin(), testParty.end());
  }

  std::shuffle(train.begin(), train.end(), rnde);
  std::shuffle(test.begin(), test.end(), rnde);
}

double calculateAccuracy(const std::vector<Person>& train, const std::vector<Person>& test) {
  NaiveBayesClassifier classifier(train);
  classifier.train();

  size_t predictions = 0;
  for (const Person& individual : test) {
    if (classifier.predict(individual)) {
      predictions++;
    }
  }

  return (double)(predictions) / test.size();
}

std::vector<double> calculateKFoldAccuracy(const std::vector<Person>& dataset, const size_t K, double& mean, double& stdev) {
  const size_t testSize = dataset.size() / K;

  std::vector<double> accuracies;
  for (size_t i = 0; i < K; i++) {
    std::vector<Person> test;
    std::vector<Person> train;
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

void solve(const std::vector<Person>& dataset) {
  std::vector<Person> train, test;
  splitTrainTest(dataset, train, test, 0.8);

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

    const int mode = argc >= 3 ? std::atoi(argv[2]) : 0;
    const CSV csv = CSVReader::readFile(argv[1]);
    const std::vector<Person> dataset = DatasetReader::readCSV(csv, mode);
    solve(dataset);
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
