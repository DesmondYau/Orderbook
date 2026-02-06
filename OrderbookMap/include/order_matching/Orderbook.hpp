#pragma once

#include "Trade.hpp"
#include "Order.hpp"
#include "OrderbookLevelInfos.hpp"
#include "OrderModify.hpp"
#include <map>
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

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool canMatch(Side side, Price price) const;
    Trades matchOrder();
};