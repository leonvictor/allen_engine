#include "reflected_types/reflected_type_editor.hpp"

namespace aln
{
std::unordered_map<StringID, IPrimitiveTypeEditor*> ReflectedTypeEditor::PrimitiveTypeEditors;

void ReflectedTypeEditor::Shutdown()
{
    for (auto& [id, pEditor] : PrimitiveTypeEditors)
    {
        aln::Delete(pEditor);
    }
    PrimitiveTypeEditors.clear();
}
} // namespace aln