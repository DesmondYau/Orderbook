#include <gtest/gtest.h>
#include <tuple>
#include "../include/order_matching/Orderbook.hpp"
#include "../include/utils/InputHandler.hpp"

namespace googletest = ::testing;


class OrderbookTestsFixture : public googletest::TestWithParam<const char*> 
{
private:
    const static inline std::filesystem::path Root{ std::filesystem::current_path() };
    const static inline std::filesystem::path TestFolder{ "testFiles" };
public:
    const static inline std::filesystem::path TestFolderPath{ Root / TestFolder };
};

TEST_P(OrderbookTestsFixture, OrderbookTestSuite)
{
    // Arrange
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam();

    InputHandler handler;
    const auto [informations, result] = handler.getInformations(file);

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
    for (const auto& action : informations)
    {
        switch (action.actionType_)
        {
        case ActionType::Add:
        {
            const Trades& trades = orderbook.addOrder(getOrder(action));
        }
        break;
        case ActionType::Modify:
        {
            const Trades& trades = orderbook.modifyOrder(getOrderModify(action));
        }
        break;
        case ActionType::Cancel:
        {
            orderbook.cancelOrder(action.orderId_);
        }
        break;
        default:
            throw std::logic_error("Unsupported Action.");
        }
    }

    // Assert
    const auto& orderbookInfos = orderbook.getOrderInfos();
    ASSERT_EQ(orderbook.size(), result.allCount_);
    ASSERT_EQ(orderbookInfos.getBids().size(), result.bidCount_);
    ASSERT_EQ(orderbookInfos.getAsks().size(), result.askCount_);
}

INSTANTIATE_TEST_CASE_P(Tests, OrderbookTestsFixture, googletest::ValuesIn({
    "cancel_success.txt",
    "match_goodTillCancel.txt",
    "match_market.txt",
    "modify_side.txt"
}));
