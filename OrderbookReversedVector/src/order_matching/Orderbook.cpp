#include "../../include/order_matching/Orderbook.hpp"
#include <numeric>
#include <algorithm>
#include <iostream>


Trades Orderbook::addOrder(OrderPointer order)
{
    if (orders_.contains(order->getOrderID()))
        return {};

    if (order->getOrderType() == OrderType::Market)
    {
        if (order->getSide() == Side::Buy && !asks_.empty())
        {
            const auto& [worstAsk, _] = *asks_.begin();
            order->toGoodTillCancel(worstAsk);
        } 
        else if (order->getSide() == Side::Sell && !bids_.empty())
        {
            const auto& [worstBid, _] = *bids_.begin();
            order->toGoodTillCancel(worstBid);
        } 
        else
            return {};
    }
    
    if (order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice()))
        return {};

    OrderPointers::iterator iterator;

    if (order->getSide() == Side::Buy)
    {
        auto it = std::lower_bound(
            bids_.begin(), 
            bids_.end(), 
            order->getPrice(),
            [](const auto& priceLevel, const Price& price) {
                return priceLevel.first < price;
            } 
        );

        if (it != bids_.end() && it->first == order->getPrice())
        {
            it->second.push_back(order);
            iterator = std::prev(it->second.end());
        }
        else
        {
            OrderPointers orderPointers;
            orderPointers.push_back(order);
            iterator = orderPointers.begin();
            bids_.insert(it, std::make_pair(order->getPrice(), std::move(orderPointers)));
        };
    }
    else
    if (order->getSide() == Side::Sell)
    {
        auto it = std::lower_bound(
            asks_.begin(), 
            asks_.end(), 
            order->getPrice(), 
            [](const auto& priceLevel, const Price& price) { 
                return priceLevel.first > price;
            } 
        ); 
        
        if (it != asks_.end() && it->first == order->getPrice()) 
        {
            it->second.push_back(order);
            iterator = std::prev(it->second.end());
        }
        else 
        {
            OrderPointers orderPointers; 
            orderPointers.push_back(order); 
            iterator = orderPointers.begin();
            asks_.insert(it, std::make_pair(order->getPrice(), std::move(orderPointers))); 
        }
    }

    orders_.insert({order->getOrderID(), OrderEntry{ order, iterator }});
    return matchOrder();
    
}


void Orderbook::cancelOrder (OrderId orderId)
{
    if (!orders_.contains(orderId))
        return;
    
    const auto& [order, orderIterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->getSide() == Side::Sell)
    {
        auto price = order->getPrice();
        auto priceLevelIt = std::find_if(
            asks_.begin(),
            asks_.end(),
            [price](const auto& priceLevel) {
                return priceLevel.first == price;
            }
        );
        if (priceLevelIt == asks_.end())
        {
            throw std::logic_error(std::format("Cancel failed: price level {} not found for order {}", price, orderId));
        }

        auto& orderPointers = priceLevelIt->second;
        orderPointers.erase(orderIterator);

        if (orderPointers.empty())
            asks_.erase(priceLevelIt);
    }
    else
    {
        auto price = order->getPrice();
        auto priceLevelIt = std::find_if(
            bids_.begin(),
            bids_.end(),
            [price](const auto& priceLevel) {
                return priceLevel.first == price;
            }
        );
        if (priceLevelIt == bids_.end())
        {
            throw std::logic_error(std::format("Cancel failed: price level {} not found for order {}", price, orderId));
        }

        auto& orderPointers = priceLevelIt->second;
        orderPointers.erase(orderIterator);

        if (orderPointers.empty())
            bids_.erase(priceLevelIt);
    }
}

Trades Orderbook::modifyOrder(OrderModify order)
{
    if (!orders_.contains(order.getOrderId()))
        return {};
    
    const auto& [existingOrder, _] = orders_.at(order.getOrderId());
    cancelOrder(order.getOrderId());
    return addOrder(order.toOrderPointer(existingOrder->getOrderType()));
}

std::size_t Orderbook::size() const { return orders_.size(); }
    
OrderbookLevelInfos Orderbook::getOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
    {
        return LevelInfo{
            price,
            std::accumulate(orders.begin(), orders.end(), (Quantity)0,
                [](Quantity runningSum, const OrderPointer& order)
                { return runningSum + order->getRemainingQuantity(); })
        };
    };

    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));
    
    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    return OrderbookLevelInfos{ bidInfos, askInfos };
}




bool Orderbook::canMatch(Side side, Price price) const
{
    if (side == Side::Buy)
    {
        if (asks_.empty())
            return false;

        const auto& bestAsk = asks_.back().first;
        return price >= bestAsk;
    }
    else
    {
        if (bids_.empty())
            return false;
        
        const auto& bestBid = bids_.back().first;
        return price <= bestBid;
    }
}


Trades Orderbook::matchOrder()
{
    Trades trades;
    trades.reserve(orders_.size());
    
    while (true)
    {
        if (bids_.empty() || asks_.empty())
            break;

        auto& [bestBid, bestBidOrders] = bids_.back();
        auto& [bestAsk, bestAskOrders] = asks_.back();

        if (bestBid < bestAsk)
            break;
        
        while (!bestBidOrders.empty() && !bestAskOrders.empty())
        {
            auto& bidOrder = bestBidOrders.front();
            auto& askOrder = bestAskOrders.front();
            
            Quantity quantity = std::min(bidOrder->getRemainingQuantity(), askOrder->getRemainingQuantity());
            bidOrder->fill(quantity);
            askOrder->fill(quantity);
            
            trades.push_back(Trade(
                TradeInfo{ bidOrder->getOrderID(), bidOrder->getPrice(), quantity },
                TradeInfo{ askOrder->getOrderID(), askOrder->getPrice(), quantity }
            ));
            
            if (bidOrder->isFilled())
            {
                bestBidOrders.pop_front();
                orders_.erase(bidOrder->getOrderID());
            }
            
            if (askOrder->isFilled())
            {
                bestAskOrders.pop_front();
                orders_.erase(askOrder->getOrderID());
            }
            
            if (bestBidOrders.empty())
                bids_.pop_back(); 
            
            if (bestAskOrders.empty())
                asks_.pop_back();
            
        }
        
        if (!bids_.empty())
        {
            auto& [_, bestBidOrders] = bids_.back();
            auto& order = bestBidOrders.front();
            if (order->getOrderType() == OrderType::FillAndKill)
                cancelOrder(order->getOrderID());                   
        }
        

        if (!asks_.empty())
        {
            auto& [_, bestAskOrders] = asks_.back();
            auto& order = bestAskOrders.front();
            if (order->getOrderType() == OrderType::FillAndKill)   
                cancelOrder(order->getOrderID());
            
        }
    }
    return trades;
}


OrderId Orderbook::getRandomOrderId(std::mt19937& rng) const
{
    if (orders_.empty()) {
        throw std::logic_error("No orders available");
    }

    // Pick a random index in [0, size-1]
    std::uniform_int_distribution<size_t> dist(0, orders_.size() - 1);
    size_t index = dist(rng);

    auto it = orders_.begin();
    std::advance(it, index); 
    return it->first;         
}
