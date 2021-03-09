namespace vkg
{
class Renderer
{
    enum State
    {
        Uninitialized,
        Initialized
    };

    State m_state = State::Uninitialized;

  public:
    void Initialize()
    {
        // What are the requirements ?
    }

    void BeginFrame()
    {
    }

    void EndFrame()
    {
    }
};
} // namespace vkg