
namespace Utils
{
    template <typename TIn, typename TOut>
    TOut map(TIn in, TIn old_min, TIn old_max, TOut new_min, TOut new_max)
    {
        return static_cast<TOut>(in * (new_max - new_min) / (old_max - old_min));
    }
}
