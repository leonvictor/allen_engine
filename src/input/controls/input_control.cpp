#include "input_control.hpp"

bool operator<(const IInputControl& left, const IInputControl& right)
{
    return left.m_id < right.m_id;
}