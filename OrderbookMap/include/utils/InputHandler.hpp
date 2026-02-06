#include "../order_matching/Types.hpp"
#include "../order_matching/Order.hpp"
#include <vector>
#include <ranges>
#include <fstream>
#include <filesystem>

enum class ActionType
{
    Add,
    Cancel,
    Modify
};

std::string toString(ActionType type)
{
    switch (type)
    {
    case ActionType::Add:    return "Add";
    case ActionType::Modify: return "Modify";
    case ActionType::Cancel: return "Cancel";
    default:                 return "Unknown";
    }
}

// A data structure to hold our action / incremental state application for testing
struct Information
{
    ActionType actionType_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;


// A data structure to hold the final state of our orderbook to compare against
struct Result
{
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

// Handler to convert input from files into incremental update applied on our orderbook for testing
class InputHandler
{
private:
    // converts string to uint32_t safely.
    std::uint32_t ToNumber(const std::string_view& str) const
    {
        std::int64_t value {};
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0)
            throw std::logic_error("value is below zero.");
        return static_cast<std::uint32_t>(value);
    }

    // parse final result line into Result data structure
    bool tryParseResult(const std::string_view& str, Result& result) const
    {
        if (str.at(0) != 'R')
            return false;
        
        auto values = split(str, ' ');
        result.allCount_ = ToNumber(values[1]);
        result.bidCount_ = ToNumber(values[2]);
        result.askCount_ = ToNumber(values[3]);

        return true;
    }

    
    // parse action lines in the test files to Information data structure
    bool tryParseInformation(const std::string_view& str, Information& information) const
    {
        auto value = str.at(0);
        auto values = split(str, ' ');

        if (value == 'A')
        {
            information.actionType_ = ActionType::Add;
            information.side_ = parseSide(values[1]);
            information.orderType_ = parseOrderType(values[2]);
            information.price_ = parsePrice(values[3]);
            information.quantity_ = parseQuantity(values[4]);
            information.orderId_ = parseOrderId(values[5]);
        }
        else if (value == 'M')
        {
            information.actionType_ = ActionType::Modify;
            information.orderId_ = parseOrderId(values[1]);
            information.side_ = parseSide(values[2]);
            information.price_ = parsePrice(values[3]);
            information.quantity_ = parsePrice(values[4]);
        }
        else if (value == 'C')
        {
            information.actionType_ = ActionType::Cancel;
            information.orderId_ = parseOrderId(values[1]);
        }
        else
            return false;
        
        return true;
    }
    

    std::vector<std::string_view> split(const std::string_view& str, char delimiter) const
    {
        std::vector<std::string_view> vec;
        vec.reserve(5);
        for (const auto&& word_range : std::views::split(str, delimiter))
        {
            std::string_view token(&*word_range.begin(), static_cast<std::size_t>(std::ranges::distance(word_range)));
            vec.push_back(token);
        }
        return vec;
    }
    
    Side parseSide(const std::string_view& str) const
    {
        if (str == "B")
            return Side::Buy;
        else if (str == "S")
            return Side::Sell;
        else
            throw std::logic_error("Unknown Side");
    }

    OrderType parseOrderType(const std::string_view str) const
    {
        if (str == "Market")
            return OrderType::Market;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else
            throw std::logic_error("Unknown OrderType");
    }

     Price parsePrice(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Price");

        return ToNumber(str);
    }

    Quantity parseQuantity(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity");

        return ToNumber(str);
    }

    OrderId parseOrderId(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Empty OrderId");

        return static_cast<OrderId>(ToNumber(str));
    }

public:
    std::tuple<Informations, Result> getInformations(const std::filesystem::path& path) const
    {
        Informations informations;
        informations.reserve(1000);

        std::string line;
        std::ifstream file { path };
        while (std::getline(file, line))
        {
            if (line.empty())
                break;

            const bool isResult = line.at(0) == 'R';
            const bool isInformation = !isResult;

            if (isInformation)
            {
                Information information;

                auto isValid = tryParseInformation(line, information);
                if (!isValid)
                    continue;

                informations.push_back(information);
            }
            else{
                if (!file.eof())
                    throw std::logic_error("Result should only be specified at the end.");

                Result result;

                auto isValid = tryParseResult(line, result);
                if (!isValid)
                    continue;

                return { informations, result };
            }
        }
        Result defaultResult;
        return { informations, defaultResult };
    }
};