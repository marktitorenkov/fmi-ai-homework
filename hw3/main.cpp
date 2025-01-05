#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

std::random_device rndd;
std::default_random_engine rnde(rndd());

int randint(int i, int j) {
    std::uniform_int_distribution<int> dist(i, j - 1);
    return dist(rnde);
}

size_t randindex(size_t size) {
    std::uniform_int_distribution<size_t> dist(0, size - 1);
    return dist(rnde);
}

double randdouble(double i, double j) {
    std::uniform_real_distribution<double> dist(i, j);
    return dist(rnde);
}

class City {
private:
    std::string name;
    double x;
    double y;
public:
    City(const std::string& name, double x, double y) :
    name(name), x(x), y(y)
    {}

    double distance(const City &other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    std::string getName() const {
        return name;
    }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }
};

class Individual {
private:
    std::vector<const City*> cities;
    double fitness;
public:
    Individual()
    : cities(), fitness(0) {}
    Individual(const std::vector<City>& arr)
    : cities(ctor(arr)), fitness(totalDistance()) {}
    Individual(const std::vector<const City*>& arr)
    : cities(arr), fitness(totalDistance()) {}

    static std::vector<const City*> ctor(const std::vector<City>& arr) {
        std::vector<const City*> cities;
        for (const City& city : arr) {
            cities.push_back(&city);
        }
        return cities;
    }

    double getFitness() const {
        return fitness;
    }

    Individual shuffled () const {
        std::vector<const City*> cpy = cities;
        std::shuffle(cpy.begin(), cpy.end(), rnde);
        return {cpy};
    }

    Individual mutated() const {
        if (randdouble(0, 1) < 0.5) {
            return mutated_swap();
        } else {
            return mutated_reverse();
        }
    }

    std::vector<City> getCities() const {
        std::vector<City> copy;
        for (const City* c : cities) {
            copy.push_back(*c);
        }
        return copy;
    }

    bool operator<(const Individual& other) const {
        return this->getFitness() < other.getFitness();
    }

    bool operator>(const Individual& other) const {
        return this->getFitness() > other.getFitness();
    }

    bool operator==(const Individual& other) const {
        return this->cities == other.cities;
    }

    static Individual crossover(const Individual& parent1, const Individual& parent2) {
        std::vector<const City*> childCities;
        std::unordered_set<const City*> inChild;

        size_t crossoverPoint = randindex(parent1.cities.size());

        for (size_t i = 0; i < crossoverPoint; ++i) {
            childCities.push_back(parent1.cities[i]);
            inChild.insert(parent1.cities[i]);
        }

        for (const City* city : parent2.cities) {
            if (!inChild.contains(city)) {
                childCities.push_back(city);
                inChild.insert(city);
            }
        }

        return {childCities};
    }

private:
    Individual mutated_swap() const {
        std::vector<const City*> cpy = cities;
        size_t i = randindex(cities.size());
        size_t j = randindex(cities.size());
        std::swap(cpy[i], cpy[j]);
        return {cpy};
    }

    Individual mutated_reverse() const {
        std::vector<const City*> cpy = cities;
        size_t i = randindex(cities.size());
        size_t j = randindex(cities.size());
        if (i > j) std::swap(i, j);
        std::reverse(cpy.begin() + i, cpy.begin() + j);
        return {cpy};
    }

    double totalDistance() const {
        double dist = 0.0;
        for (size_t i = 0; i < cities.size() - 1; ++i) {
            const City& a = *cities[i];
            const City& b = *cities[i + 1];
            dist += a.distance(b);
        }
        return dist;
    }
};

struct Result {
    const Individual finalBest;
    const std::vector<double> bestPerGen;
};

class Solver {
private:
    const int populationSize;
    const int truncatedSize;
    const int tournamentSize;
    const int noImprovementMax;
    const double mutationRate;

public:
    Solver(int populationSize = 5000,
           double selectionFactor = 0.75,
           double tournamentSize = 3,
           int convergenceThreshold = 15,
           double mutationRate = 0.1) :
    populationSize(populationSize),
    truncatedSize(populationSize * selectionFactor),
    tournamentSize(tournamentSize),
    noImprovementMax(convergenceThreshold),
    mutationRate(mutationRate)
    {}

    Result solve(const std::vector<City>& cities) {
        Individual seed(cities);
        return geneticAlgorithm(seed);
    }

private:
    Result geneticAlgorithm(const Individual& seed) {
        std::vector<double> bestPerGen;
        std::vector<Individual> population;
        for (size_t i = 0; i < populationSize; ++i) {
            population.push_back(seed.shuffled());
        }

        double prevBest = std::numeric_limits<double>::max();
        int noImprovement = 0;
        do {
            std::sort(population.begin(), population.end());

            double best = population.front().getFitness();
            bestPerGen.push_back(best);
            if (best == prevBest) {
                noImprovement++;
            } else {
                prevBest = best;
                noImprovement = 0;
            }

            population.resize(truncatedSize);

            while (population.size() < populationSize) {
                const Individual& parent1 = tournamentSelection(population);
                const Individual& parent2 = tournamentSelection(population);
                Individual child = Individual::crossover(parent1, parent2);

                if (randdouble(0, 1) < mutationRate) {
                    population.push_back(child.mutated());
                } else {
                    population.push_back(child);
                }
            }
        } while(noImprovement < noImprovementMax);

        Individual bestIndiv = *std::min_element(population.begin(), population.end());

        return {bestIndiv, bestPerGen};
    }

    Individual tournamentSelection(const std::vector<Individual>& population) {
        std::vector<Individual> tournamentPool;
        for (int i = 0; i < tournamentSize; ++i) {
            tournamentPool.push_back(population[randindex(truncatedSize)]);
        }
        return *std::min_element(tournamentPool.begin(), tournamentPool.end());
    }
};

std::vector<City> genCities(int N, int xyrange = 500) {
    std::vector<City> cities;
    cities.reserve(N);

    for (int i = 0; i < N; ++i) {
        double x = randdouble(0, xyrange);
        double y = randdouble(0, xyrange);
        cities.emplace_back("", x, y);
    }

    return cities;
}

std::vector<City> readCities(const std::string& dataset) {
    using namespace std;

    string nameFile = dataset + "_name.csv";
    string xyFile = dataset + "_xy.csv";

    ifstream nameStream(nameFile);
    ifstream xyStream(xyFile);

    if (!nameStream || !xyStream) {
        cerr << "Error opening file." << endl;
        return {};
    }

    vector<City> cities;
    string nameLine;
    string xyLine;
    while (getline(nameStream, nameLine) && getline(xyStream, xyLine)) {
        double x, y;
        stringstream ss(xyLine);
        if (ss >> x && ss.ignore(1) && ss >> y) {
            cities.emplace_back(nameLine, x, y);
        } else {
            cerr << "Error reading xy " << xyLine << endl;
            return {};
        }
    }

    return cities;
}

void printResult(const Result& result, int printSteps = 8) {
    using namespace std;

    const int n = (int)result.bestPerGen.size();
    const int s = max({1, n / printSteps});
    for (int i = 0; i < n; i++) {
        if (i == 0 || i == n - 1 || i % s == 0) {
            cout << result.bestPerGen[i] << endl;
        }
    }
    cout << endl;

    auto cities = result.finalBest.getCities();
    for (int i = 0; i < cities.size(); i++) {
        const City& c = cities[i];
        if (!c.getName().empty()) {
            cout << c.getName();
        } else {
            cout << "(" << c.getX() << ", " << c.getY() << ")";
        }
        if (i != cities.size() - 1) {
            cout << " -> ";
        }
    }
    cout << endl;
    cout << result.bestPerGen.back() << endl;
}

int main(int argc, const char* argv[]) {
    using namespace std;
    using namespace std::chrono;

    bool isAutomatedTest = !(argc >= 2 && argv[1] == string("-t"));

    int N;
    std::string input;
    std::getline(std::cin, input);
    std::stringstream ss(input);

    std::vector<City> cities = ss >> N
        ? genCities(N)
        : readCities(input);
    
    if (cities.empty()) return -1;

    auto start = high_resolution_clock::now();
    auto result = Solver().solve(cities);
    auto stop = high_resolution_clock::now();
    double time = duration<double>(stop - start).count();
    
    printResult(result);

    if (!isAutomatedTest) {
        cout << "Exection time: " << time << "s" << endl;
    }

    return 0;
}
