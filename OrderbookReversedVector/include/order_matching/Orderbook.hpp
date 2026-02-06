#pragma once

#include "Trade.hpp"
#include "Order.hpp"
#include "OrderbookLevelInfos.hpp"
#include "OrderModify.hpp"
#include <vector>
#include <unordered_map>
#include <random>




class Orderbook
{
public:
    Trades addOrder(OrderPointer order);
    void cancelOrder (OrderId orderId);
    Trades modifyOrder(OrderModify order);
    std::size_t size() const;
    OrderbookLevelInfos getOrderInfos() const;

    OrderId getRandomOrderId(std::mt19937& rng) const;               // For generating orders for performance testing
    
private:
    struct OrderEntry
    { 
        OrderPointer orderptr_ { nullptr };
        OrderPointers::iterator location_;
    };

    std::vector<std::pair<Price, OrderPointers>> bids_;
    std::vector<std::pair<Price, OrderPointers>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool canMatch(Side side, Price price) const;
    Trades matchOrder();
};