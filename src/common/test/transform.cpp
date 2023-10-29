#include <catch2/catch_test_macros.hpp>

#include <common/transform.hpp>
#include <common/maths/vec3.hpp>
#include <common/maths/quaternion.hpp>
#include <common/maths/matrix4x4.hpp>


namespace aln
{
TEST_CASE("Identity Transform", "[transform]")
{
    SECTION("Identity is correct")
    {
        Transform identity = Transform::Identity;
        REQUIRE(identity.GetTranslation() == Vec3(0.0f, 0.0f, 0.0f));
        REQUIRE(identity.GetRotation() == Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        REQUIRE(identity.GetScale() == Vec3(1.0f, 1.0f, 1.0f));
    }
}

TEST_CASE("Transform equality", "[transform]")
{
    auto q = Quaternion(0.5f, 0.6f, 0.5f, 0.8f).Normalized();
    auto a = Transform(Vec3(52.0f, 8.0f, 7.0f), q, Vec3(5.0f, 2.0f, 3.0f));
    auto b = Transform(Vec3(52.0f, 8.0f, 7.0f), q, Vec3(5.0f, 2.0f, 3.0f));
    REQUIRE(a == b);
}

TEST_CASE("Transforms to and from matrices", "[transform]")
{
    auto q = Quaternion(0.5f, 0.6f, 0.5f, 0.8f).Normalized();
    auto a = Transform(Vec3(52.0f, 8.0f, 7.0f), q, Vec3(5.0f, 2.0f, 3.0f));
    auto mat = a.ToMatrix();
    auto b = mat.ToTransform();

    REQUIRE(a == b);
}

TEST_CASE("Transforms multiplication", "[transform]")
{
    SECTION("Transform * Identity")
    {
        Transform t = Transform(Vec3(-2.144487f, 3.263043f, -5.987485f), Quaternion(1.0f, 0.5f, 0.2f, 0.8f).Normalized(), Vec3(1.0f, 1.0f, 1.0f));
        REQUIRE(t == Transform::Identity * t);
        REQUIRE(t == t * Transform::Identity);
    }

    SECTION("Parent * Child")
    {
        Transform parent = Transform(Vec3(1.0f, 1.0f, 1.0f), Quaternion::FromEulerAngles(EulerAnglesDegrees(90.0f, 0.0f, 0.0f).ToRadians()).Normalized(), Vec3(1.0f, 1.0f, 1.0f));
        Transform child = Transform(Vec3(0.0f, 5.0f, 0.0f), Quaternion::FromEulerAngles(EulerAnglesDegrees(0.0f, 35.0f, 0.0f).ToRadians()).Normalized(), Vec3(1.0f, 1.0f, 1.0f));
        Transform expectedChildGlobal = Transform(Vec3(1.0f, 1.0f, 6.0f), Quaternion::FromEulerAngles(EulerAnglesDegrees(90.0f, 35.0f, 0.0f).ToRadians()).Normalized(), Vec3(1.0f, 1.0f, 1.0f));

        auto childGlobal = parent * child;
        REQUIRE(childGlobal == expectedChildGlobal);
    }
}

TEST_CASE("Transform inverse", "[transform]")
{
    REQUIRE(Transform::Identity.GetInverse() == Transform::Identity);

    auto t = Transform(Vec3(52.0f, -4.0f, 0.0f), Quaternion(0.5f, 0.8f, 0.1f, 0.3).Normalized(), Vec3(4.0f, 5.0f, -7.0f));
    REQUIRE(t.GetInverse() * t == Transform::Identity);
}
} // namespace aln