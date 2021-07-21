#include "command.hpp"
#include "entity.hpp"

namespace aln::entities
{

void Command::Execute(Entity* pEntity)
{
    m_updatedEntities.push_back(pEntity);
}
} // namespace aln::entities