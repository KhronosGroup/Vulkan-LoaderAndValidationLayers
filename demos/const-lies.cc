#include <sys/mman.h>
#include <vulkan/vulkan.h>

/* helpers */
template<typename T>
size_t size_rounded_to_pages() { return (sizeof(T) + 0x3ff) & ~0x3ff; }

template<typename T>
T* alloc_protectable() {
    return (T*)mmap(0,
                    size_rounded_to_pages<T>(),
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,
                    0);
}

template<typename T>
void protect(T *t) {
    mprotect(t, size_rounded_to_pages<T>(),
             PROT_READ);
}

template<typename T>
void unprotect(T *t) {
    mprotect(t, size_rounded_to_pages<T>(),
             PROT_READ | PROT_WRITE);
}

template<typename T>
void free_protectable(T *t) {
    munmap(t, size_rounded_to_pages<T>());
}

int main()
{
    auto ici = alloc_protectable<VkInstanceCreateInfo>();
    ici->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici->pNext = nullptr;
    ici->flags = 0;
    ici->pApplicationInfo = nullptr;
    ici->enabledLayerCount = 1;
    ici->enabledExtensionCount = 0;
    auto names = alloc_protectable<char const *>();
    names[0] = "VK_LAYER_LUNARG_standard_validation";
    ici->ppEnabledLayerNames = names;
    ici->ppEnabledExtensionNames = nullptr;

    protect(names);
    protect(ici);

    VkInstance inst;
    // LunarG loader is poorly behaved and scribbles on *ici!
    vkCreateInstance(ici, nullptr, &inst);

    unprotect(ici);
    unprotect(names);

    free_protectable(names);
    free_protectable(ici);

    vkDestroyInstance(inst, nullptr);
}
