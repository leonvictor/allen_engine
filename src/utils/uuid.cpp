#include <vulkan/vulkan.hpp>
#include <iomanip>
#include <iostream> 

namespace core {
    class UUID {
    public:
        UUID(uint8_t const data[VK_UUID_SIZE]) {
            memcpy(m_data, data, VK_UUID_SIZE * sizeof(uint8_t));
        }

        uint8_t m_data[VK_UUID_SIZE];
    };

    // TODO: shouldn't this be inside the class ?
    std::ostream & operator<<(std::ostream & os, UUID uuid) {
            os << std::setfill( '0' ) << std::hex;
            for ( int j = 0; j < VK_UUID_SIZE; ++j ) {
                os << std::setw( 2 ) << static_cast<uint32_t>(uuid.m_data[j]);
                if ( j == 3 || j == 5 || j == 7 || j == 9 ) {
                    std::cout << '-';
                }
            }
            os << std::setfill( ' ' ) << std::dec;
            return os;
        }
}