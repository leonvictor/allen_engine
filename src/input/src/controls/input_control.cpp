#include "controls/input_control.hpp"

namespace aln::input
{
bool operator<(const IInputControl& left, const IInputControl& right)
{
    return left.m_id < right.m_id;
}
}