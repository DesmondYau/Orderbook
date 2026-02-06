#include "../include/order_matching/Orderbook.hpp"
#include <random>
#include <fstream>

class GenerateOrder
{
public:
    void createOrders(int numberOfOrders);

private:
    Orderbook orderbook_;
    std::mt19937 rng_ {std::random_device{}()};
    std::uniform_int_distribution<int> qtyDist_ {1, 100};
    std::uniform_int_distribution<int> sideDist_ {0, 1};
    std::uniform_int_distribution<int> typeDist_ {0, 2};
    std::normal_distribution<> priceDist_ {100000, 500};
    std::discrete_distribution<int> actionDist_ { {65, 10, 25} };

    std::ofstream file_;

    void generateAdd(int id);
    void generateModify();
    void generateCancel();
};