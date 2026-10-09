#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "service/ChatProvider.h"
#include "service/MockChatProvider.h"
#include "service/GeminiChatProvider.h"
#include "service/ChatProviderFactory.h"
#include "service/ChatService.h"
#include "exception/ApiException.h"

using namespace dd::ddmart::service;
using namespace dd::ddmart::exception;

// -------------------------------------------------------------
// 1. MockChatProvider Unit Tests (Section 9 Rule 6 FAQ Coverage)
// -------------------------------------------------------------
TEST(MockChatProviderTest, IdentifiesProviderName) {
    MockChatProvider provider;
    EXPECT_EQ(provider.GetProviderName(), "mock");
}

TEST(MockChatProviderTest, AnswersReturnPolicyAccurately) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("What is your return policy?", "");
    EXPECT_NE(reply.find("30-day return policy"), std::string::npos);
    EXPECT_NE(reply.find("refund"), std::string::npos);
}

TEST(MockChatProviderTest, AnswersShippingQuestions) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("How long does shipping and delivery take?", "");
    EXPECT_NE(reply.find("Standard Delivery"), std::string::npos);
    EXPECT_NE(reply.find("3–5 business days"), std::string::npos);
}

TEST(MockChatProviderTest, AnswersPaymentAndCheckoutQuestions) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("What payment methods can I use at checkout?", "");
    EXPECT_NE(reply.find("mock checkout"), std::string::npos);
    EXPECT_NE(reply.find("Credit/Debit cards"), std::string::npos);
}

TEST(MockChatProviderTest, AnswersSellerOnboardingQuestions) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("How can I become a seller and list items?", "");
    EXPECT_NE(reply.find("SELLER"), std::string::npos);
    EXPECT_NE(reply.find("Seller Dashboard"), std::string::npos);
}

TEST(MockChatProviderTest, AnswersOrderTrackingQuestions) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("How do I track my order status?", "");
    EXPECT_NE(reply.find("My Orders"), std::string::npos);
    EXPECT_NE(reply.find("PENDING -> CONFIRMED -> SHIPPED -> DELIVERED"), std::string::npos);
}

TEST(MockChatProviderTest, AnswersReviewQuestions) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("How do I leave a review or star rating?", "");
    EXPECT_NE(reply.find("verified buyers"), std::string::npos);
}

TEST(MockChatProviderTest, HandlesGreetingsFriendly) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("Hello there!", "");
    EXPECT_NE(reply.find("Welcome to DDMart"), std::string::npos);
}

TEST(MockChatProviderTest, HandlesOutOfScopeQuestionsWithGuardrail) {
    MockChatProvider provider;
    std::string reply = provider.GetReply("Can you write a poem about quantum physics?", "");
    EXPECT_NE(reply.find("DDMart Marketplace Assistant"), std::string::npos);
}

// -------------------------------------------------------------
// 2. ChatService Guardrails, Validation, Caching & Rate Limiting
// -------------------------------------------------------------
class ChatServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        provider = std::make_shared<MockChatProvider>();
        service = std::make_unique<ChatService>(provider);
    }

    std::shared_ptr<MockChatProvider> provider;
    std::unique_ptr<ChatService> service;
};

TEST_F(ChatServiceTest, ValidatesEmptyOrWhitespaceMessage) {
    EXPECT_THROW(service->ProcessMessage("sess_1", ""), ValidationException);
    EXPECT_THROW(service->ProcessMessage("sess_1", "   \t\n  "), ValidationException);
}

TEST_F(ChatServiceTest, ValidatesMessageLengthExceeded) {
    std::string oversized(501, 'a');
    EXPECT_THROW(service->ProcessMessage("sess_1", oversized), ValidationException);

    std::string valid_len(500, 'b');
    EXPECT_NO_THROW(service->ProcessMessage("sess_1", valid_len));
}

TEST_F(ChatServiceTest, CachesIdenticalQuestionsWithinSession) {
    std::string sess = "cache_sess_100";
    std::string query = "What is your return policy?";

    // First call: fresh query
    auto resp1 = service->ProcessMessage(sess, query);
    EXPECT_FALSE(resp1.cached);
    EXPECT_NE(resp1.reply.find("30-day return policy"), std::string::npos);

    // Second call with identical question: must hit cache
    auto resp2 = service->ProcessMessage(sess, query);
    EXPECT_TRUE(resp2.cached);
    EXPECT_EQ(resp1.reply, resp2.reply);

    // Third call with different casing/trailing space: normalized cache hit
    auto resp3 = service->ProcessMessage(sess, "  what is your return policy?  ");
    EXPECT_TRUE(resp3.cached);
    EXPECT_EQ(resp1.reply, resp3.reply);
}

TEST_F(ChatServiceTest, EnforcesPerSessionRateLimitTenPerMinute) {
    std::string sess = "rate_limit_sess_200";

    // 10 queries in sequence should succeed
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(service->CheckRateLimit(sess));
        auto resp = service->ProcessMessage(sess, "Question number " + std::to_string(i));
        EXPECT_FALSE(resp.reply.empty());
    }

    // 11th query in same minute window must be rejected with 429
    EXPECT_FALSE(service->CheckRateLimit(sess));
    try {
        service->ProcessMessage(sess, "Question 11 - should fail");
        FAIL() << "Expected ApiException with 429 status";
    } catch (const ApiException& e) {
        EXPECT_EQ(e.GetStatusCode(), 429);
        EXPECT_EQ(e.GetErrorCode(), "RATE_LIMIT_EXCEEDED");
    }
}

TEST_F(ChatServiceTest, SessionIsolationForRateLimiting) {
    std::string sess_a = "sess_user_alpha";
    std::string sess_b = "sess_user_beta";

    // Exhaust quota for session A
    for (int i = 0; i < 10; ++i) {
        service->ProcessMessage(sess_a, "Alpha Query " + std::to_string(i));
    }
    EXPECT_THROW(service->ProcessMessage(sess_a, "Alpha Overflow"), ApiException);

    // Session B must remain unaffected and operate normally
    EXPECT_TRUE(service->CheckRateLimit(sess_b));
    EXPECT_NO_THROW(service->ProcessMessage(sess_b, "Beta Query 1"));
}

// -------------------------------------------------------------
// 3. Fallback & Degraded Mode Tests
// -------------------------------------------------------------
class FailingChatProvider : public ChatProvider {
public:
    std::string GetProviderName() const override { return "failing"; }
    std::string GetReply(const std::string&, const std::string&) override {
        throw std::runtime_error("Simulated outbound network timeout failure");
    }
};

TEST(ChatServiceDegradationTest, RecoversFromProviderExceptionWithDegradedResponse) {
    auto failing_provider = std::make_shared<FailingChatProvider>();
    ChatService resilient_service(failing_provider);

    auto resp = resilient_service.ProcessMessage("resilient_sess", "Hello support");
    EXPECT_FALSE(resp.reply.empty());
    EXPECT_NE(resp.reply.find("offline assistance mode"), std::string::npos);
}

// -------------------------------------------------------------
// 4. ChatProviderFactory Pattern Tests (Section 10 Factory Pattern)
// -------------------------------------------------------------
TEST(ChatProviderFactoryTest, CreatesMockProviderByDefault) {
    auto provider = ChatProviderFactory::Create("mock");
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->GetProviderName(), "mock");
}

TEST(ChatProviderFactoryTest, CreatesGeminiProviderWhenConfigured) {
    auto provider = ChatProviderFactory::Create("gemini");
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->GetProviderName(), "gemini");
}
