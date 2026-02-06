#pragma once

#include "Types.hpp"
#include <stdexcept>
#include <format>
#include <memory>
#include <list>


enum class OrderType
{
    Market,
    GoodTillCancel,
    FillAndKill
};

enum class Side
{
    Buy,
    Sell
};

class Order
{
public:
    Order(OrderId orderId, OrderType orderType, Side side, Price price, Quantity quantity)
        : orderID_ { orderId }
        , orderType_ { orderType }
        , side_ { side }
        , price_ { price }
        , initialQuantity_ { quantity }
        , remainingQuantity_ { quantity }
    {}

    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(orderId, OrderType::Market, side, Constants::InvalidPrice, quantity)
    {}

    OrderId getOrderID() const { return orderID_; }
    OrderType getOrderType() const { return orderType_; }
    Side getSide() const { return side_; }
    Price getPrice() const { return price_; }
    Quantity getInitialQuantity() const { return initialQuantity_; }
    Quantity getRemainingQuantity() const { return remainingQuantity_; }
    bool isFilled() const { return getRemainingQuantity() == 0; }
    void fill(Quantity quantity)
    {
        if (quantity > getRemainingQuantity())
            throw std::logic_error(std::format("Order {} cannot be filled for more than its remaining quantity", getOrderID()));
        remainingQuantity_ -= quantity;
    }
    void toGoodTillCancel(Price price)
    {
        if (getOrderType() != OrderType::Market)
        {
            throw std::logic_error(std::format("Order {} cannot have its price adjusted, only market orders can", getOrderID()));
        }
        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }

private:
    OrderId orderID_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;