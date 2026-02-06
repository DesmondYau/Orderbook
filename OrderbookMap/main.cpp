#include "order_matching/Orderbook.hpp"
#include "utils/InputHandler.hpp"
#include "benchmarking/GenerateOrder.hpp"
#include <iostream>
#include <chrono>
#include <string>


int main()
{  
    /*
    GenerateOrder generateOrder;
    generateOrder.createOrders(1150000);
    */
    

    std::ofstream csvFile("benchmark_time_map.csv", std::ios::trunc);
    if (!csvFile)
    {
        std::cerr << "Error opening file";
        return 1;
    }

    InputHandler handler;
    const auto [informations, result] = handler.getInformations("generatedOrders.txt");

    auto getOrder = [](const Information& information)
    {
        return std::make_shared<Order>(
            information.orderId_,
            information.orderType_,
            information.side_,
            information.price_,
            information.quantity_);
    };

    auto getOrderModify = [](const Information& information)
    {
        return OrderModify
        {
            information.orderId_,
            information.side_,
            information.price_,
            information.quantity_,
        };
    };

    // Act
    Orderbook orderbook;
    int index = 0;
    for (const auto& information : informations)
    {
        std::chrono::nanoseconds duration{0};

        switch (information.actionType_)
        {
        case ActionType::Add:
        {
            auto order = getOrder(information);
            auto start = std::chrono::steady_clock::now();
            const Trades &trades = orderbook.addOrder(order);
            auto end = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        }
        break;
        case ActionType::Modify:
        {
            auto orderMod = getOrderModify(information);
            auto start = std::chrono::steady_clock::now();
            const Trades &trades = orderbook.modifyOrder(orderMod);
            auto end = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        }
        break;
        case ActionType::Cancel:
        {
            auto start = std::chrono::steady_clock::now();
            orderbook.cancelOrder(information.orderId_);
            auto end = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        }
        break;
        default:
            throw std::logic_error("Unsupported Action.");
        }
        index++;

        csvFile << toString(information.actionType_) << "," << duration.count() << std::endl;
    }

    return 0;
}