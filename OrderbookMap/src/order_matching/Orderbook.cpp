#include "../../include/order_matching/Orderbook.hpp"
#include <numeric>


Trades Orderbook::addOrder(OrderPointer order)
{
    if (orders_.contains(order->getOrderID()))
        return {};

    if (order->getOrderType() == OrderType::Market)
    {
        if (order->getSide() == Side::Buy && !asks_.empty())
        {
            const auto& [worstAsk, _] = *asks_.rbegin();
            order->toGoodTillCancel(worstAsk);
        } 
        else if (order->getSide() == Side::Sell && !bids_.empty())
        {
            const auto& [worstBid, _] = *bids_.rbegin();
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
        auto& orders = bids_[order->getPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    if (order->getSide() == Side::Sell)
    {
        auto& orders = asks_[order->getPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({order->getOrderID(), { order, iterator}});
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
        auto& orders = asks_.at(price);
        orders.erase(orderIterator);
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        auto price = order->getPrice();
        auto& orders = bids_.at(price);
        orders.erase(orderIterator);
        if (orders.empty())
            bids_.erase(price);
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

        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else
    {
        if (bids_.empty())
            return false;
        
        const auto& [bestBid, _] = *bids_.begin();
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

        auto& [bestBid, bids] = *bids_.begin();
        auto& [bestAsk, asks] = *asks_.begin();

        if (bestBid < bestAsk)
            break;
        
        while (!bids.empty() && !asks.empty())
        {
            auto& bid = bids.front();
            auto& ask = asks.front();
            
            Quantity quantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());
            bid->fill(quantity);
            ask->fill(quantity);

            if (bid->isFilled())
            {
                bids.pop_front();
                orders_.erase(bid->getOrderID());
            }

            if (ask->isFilled())
            {
                asks.pop_front();
                orders_.erase(ask->getOrderID());
            }
            
            if (bids.empty())
                bids_.erase(bestBid); 

            if (asks.empty())
                asks_.erase(bestAsk);

            trades.push_back(Trade(
                TradeInfo{ bid->getOrderID(), bid->getPrice(), quantity },
                TradeInfo{ ask->getOrderID(), ask->getPrice(), quantity }
            ));
        }

        if (!bids_.empty())
        {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if (order->getOrderType() == OrderType::FillAndKill)
                cancelOrder(order->getOrderID());                    
        }

        if (!asks_.empty())
        {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
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
