/// @brief FIFO queue based on a ring buffer.
template <typename T>
class RingBuffer
{
    // TODO: Put in the std namespace and mimic std syntax ?
  private:
    T[] m_buffer;
    int m_head, m_tail;
    int m_size;

  public:
    RingBuffer(size_t size) : m_size(size), m_head(0), m_tail(0)
    {
        m_buffer = T[size];
    }

    /// @brief Add an element at the end of the queue.
    void Enqueue(T& element)
    {
        // TODO: Handle collapsing of identical events. A customizable collapse function might be a good idea.
        assert((m_tail + 1) % m_size != m_head, "Buffer max size reached.");

        m_buffer[m_tail] = element;
        m_tail = (m_tail + 1) % m_size;
    }

    /// @brief Get the next element in queue and remove it.
    T& Pop()
    {
        if (Empty())
        {
            return void;
        }

        T element = m_buffer[m_head];
        m_head = (m_head + 1) % m_size;
        return element;
    }

    bool Empty() const
    {
        return m_head == m_tail;
    }
};