#include <catch2/catch_test_macros.hpp>

#include <common/transform.hpp>

namespace aln
{
TEST_CASE("Identity Transform", "[transform]")
{
    SECTION("Identity is correct")
    {
        Transform identity = Transform::Identity;
        REQUIRE(identity.GetTranslation() == glm::vec3{0, 0, 0});
        REQUIRE(identity.GetRotation() == glm::quat{1, 0, 0, 0});
        REQUIRE(identity.GetScale() == glm::vec3{1, 1, 1});
    }
}

TEST_CASE("Transform equality", "[transform]")
{
    auto q = glm::normalize(glm::quat(0.5, 0.6, 0.5, 0.8));
    auto a = Transform({52, 8, 7}, q, {5, 2, 3});
    auto b = Transform({52, 8, 7}, q, {5, 2, 3});
    REQUIRE(a == b);
}

TEST_CASE("Transforms to and from matrices", "[transform]")
{
    auto q = glm::normalize(glm::quat(0.5, 0.6, 0.5, 0.8));
    auto a = Transform({52, 8, 7}, q, {5, 2, 3});
    auto mat = a.ToMatrix();
    auto b = Transform(mat);

    REQUIRE(a == b);
}

TEST_CASE("Transforms multiplication", "[transform]")
{
    SECTION("Transform * Identity")
    {
        Transform t = Transform({-2.144487, 3.263043, -5.987485}, glm::normalize(glm::quat({1, 0.5, 0.2, 0.8})), {1, 1, 1});
        REQUIRE(t == Transform::Identity * t);
        REQUIRE(t == t * Transform::Identity);
    }

    SECTION("Parent * Child")
    {
        Transform parent = Transform({1, 1, 1}, glm::normalize(glm::quat(glm::radians(glm::vec3{90, 0, 0}))), {1, 1, 1});
        Transform child = Transform({0, 5, 0}, glm::normalize(glm::quat(glm::radians(glm::vec3{0, 35, 0}))), {1, 1, 1});
        Transform expectedChildGlobal = Transform({1, 1, 6}, glm::normalize(glm::quat(glm::radians(glm::vec3{90, 35, 0}))), {1, 1, 1});

        auto childGlobal = parent * child;
        REQUIRE(childGlobal == expectedChildGlobal);
    }
}

TEST_CASE("Transform inverse", "[transform]")
{
    REQUIRE(Transform::Identity.GetInverse() == Transform::Identity);

    auto t = Transform({52, -4, 0}, glm::normalize(glm::quat(0.5, 0.8, 0.1, 0.3)), {4, 5, -7});
    REQUIRE(t.GetInverse() * t == Transform::Identity);
}
} // namespace aln