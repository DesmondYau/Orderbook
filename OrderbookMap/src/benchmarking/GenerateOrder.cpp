#include "../../include/benchmarking/GenerateOrder.hpp"
#include <random>
#include <fstream>
#include <iostream>

std::ostream& operator<<(std::ostream& os, Side s) {
    return os << (s == Side::Buy ? "B" : "S");
}

std::ostream& operator<<(std::ostream& os, OrderType t) {
    switch (t) {
        case OrderType::GoodTillCancel: return os << "GoodTillCancel";
        case OrderType::Market:         return os << "Market";
        case OrderType::FillAndKill:    return os << "FillAndKill";
    }
    return os;
}

void GenerateOrder::createOrders(int numberOfOrders)
{
    file_.open("generatedOrders.txt", std::ios::trunc);
    if (!file_) {
        std::cerr << "Error opening file\n";
        return;
    }

    for (int i=1; i <= numberOfOrders; i++)
    {
        int choice = actionDist_(rng_);
        switch (choice)
        {
            case 0: generateAdd(i); break;
            case 1: generateModify(); break;
            case 2: generateCancel(); break;
        }
    }

    file_.close();
    return;
}

void GenerateOrder::generateAdd(int id) {
    Side side { static_cast<Side>(sideDist_(rng_)) };
    OrderType orderType { static_cast<OrderType>(typeDist_(rng_)) };
    Quantity quantity { static_cast<Quantity>(qtyDist_(rng_)) };

    int price;
    if (side == Side::Buy && !orderbook_.getOrderInfos().getAsks().empty()) 
    {
        do {
            price = static_cast<int>(priceDist_(rng_));
        } while (price > orderbook_.getOrderInfos().getAsks().front().price_);
    }
    else if (side == Side::Sell && !orderbook_.getOrderInfos().getBids().empty())
    {
        do {
            price = static_cast<int>(priceDist_(rng_));
        } while (price < orderbook_.getOrderInfos().getBids().front().price_);
    }
    else
    {
        price = static_cast<int>(priceDist_(rng_));
    }

    auto order { std::make_shared<Order>(static_cast<OrderId>(id), orderType, side, price, quantity) };
    orderbook_.addOrder(order);

    file_ << "A " << side << " " << orderType << " " << price << " " << quantity << " " << id << "\n";
}

void GenerateOrder::generateModify()
{
    if (orderbook_.size() == 0)
        return;
    
    OrderId id { orderbook_.getRandomOrderId(rng_) };
    Side side { static_cast<Side>(sideDist_(rng_)) };
    Quantity quantity { static_cast<Quantity>(qtyDist_(rng_)) };
    int price;

    if (side == Side::Buy && !orderbook_.getOrderInfos().getAsks().empty()) 
    {
        do {
            price = static_cast<int>(priceDist_(rng_));
        } while (price > orderbook_.getOrderInfos().getAsks().front().price_);
    }
    else if (side == Side::Sell && !orderbook_.getOrderInfos().getBids().empty())
    {
        do {
            price = static_cast<int>(priceDist_(rng_));
        } while (price < orderbook_.getOrderInfos().getBids().front().price_);
    }
    else
    {
        price = static_cast<int>(priceDist_(rng_));
    }

    OrderModify OrderModify { static_cast<OrderId>(id), side, price, quantity };
    orderbook_.modifyOrder(OrderModify);

    file_ << "M " << id << " " << side << " " << price << " " << quantity << "\n";
}

void GenerateOrder::generateCancel()
{
    if (orderbook_.size() == 0)
        return;
    
    OrderId id { orderbook_.getRandomOrderId(rng_) };
    orderbook_.cancelOrder(id);

    file_ << "C " << id << "\n";
}

