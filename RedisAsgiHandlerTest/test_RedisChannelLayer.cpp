#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RedisChannelLayer.h"


TEST(RedisChannelLayerTest, NewChannelGivesUniqueName)
{
    RedisChannelLayer channels1, channels2;

    // Different instances give unique names.
    EXPECT_NE(channels1.NewChannel("prefix"), channels2.NewChannel("prefix"));

    // Same instance gives a unique name whenever it is called.
    EXPECT_NE(channels1.NewChannel("prefix"), channels1.NewChannel("prefix"));
}


TEST(RedisChannelLayerTest, NewChannelRespectsPrefix)
{
    RedisChannelLayer channels;

    auto prefix = std::string{"myprefix"};
    auto channel_name = channels.NewChannel(prefix);
    auto actual_prefix = channel_name.substr(0, prefix.length());

    EXPECT_EQ(prefix, actual_prefix);
}
