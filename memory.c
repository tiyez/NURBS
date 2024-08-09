
/* deps: essentials.c */

#undef __File
#define __File "memory.c"

static inline long long to_offset (_In_ _ROnly void *from, _In_ _ROnly void *to) {
    return ((char *) to - (char *) from);
}

static inline int is_inside_range (_In_ _ROnly void *ptr, _In_ _ROnly void *begin, _In_ _ROnly void *end) {
    return (ptr >= begin && ptr < end);
}

static inline int is_end_inside_range (_In_ _ROnly void *ptr, _In_ _ROnly void *begin, _In_ _ROnly void *end) {
    return (ptr >= begin && ptr <= end);
}

_Ret_maybenull_ _Check_return_ static void  *expand_memory (_In_opt_ void *memory, _In_ int *size, int minimum_size, int maximum_amount_of_pages) {
    int new_size;
    int maximum_allocation_size;

    Assert (*size == to_page_size (*size));
    maximum_allocation_size = maximum_amount_of_pages * Memory_Page;
    new_size = *size + Memory_Page;
    if (new_size < minimum_size) {
        new_size = to_page_size (minimum_size);
    }
    if (new_size > maximum_allocation_size) {
        Error ("allocation size exceeded the maximum limit (%d > %d)", new_size, maximum_allocation_size);
        memory = 0;
    } else if (memory) {
        if (0 == commit_memory_pages ((char *) memory + *size, (new_size - *size) / Memory_Page)) {
            Error ("could not commit memory pages (%p %d) 0x%x", (void *) ((char *) memory + *size), new_size - *size, GetLastError ());
            memory = 0;
        }
    } else {
        memory = reserve_virtual_address_space (maximum_amount_of_pages);
        if (memory) {
            if (0 == commit_memory_pages (memory, new_size / Memory_Page)) {
                Error ("could not commit memory pages 0x%x", GetLastError ());
                release_virtual_address_space (memory);
                memory = 0;
            }
        } else {
            Error ("could not reserve memory 0x%x", GetLastError ());
        }
    }
    if (memory) {
        *size = new_size;
    } else {
        Error ("could not expand memory");
    }
    return (memory);
}

_Ret_maybenull_ _Check_return_ static const void *expand_topdown_memory (_In_opt_ const void *memory, _In_ int *size, int minimum_size, int maximum_amount_of_pages) {
    int new_size;
    int maximum_allocation_size;

    Assert (*size == to_page_size (*size));
    maximum_allocation_size = maximum_amount_of_pages * Memory_Page;
    new_size = *size + Memory_Page;
    if (new_size < minimum_size) {
        new_size = to_page_size (minimum_size);
    }
    if (new_size > maximum_allocation_size) {
        Error ("allocation size exceeded the maximum limit (%d > %d)", new_size, maximum_allocation_size);
        memory = 0;
    } else if (memory) {
        if (0 == commit_memory_pages ((char *) memory + maximum_allocation_size - new_size, (new_size - *size) / Memory_Page)) {
            Error ("could not commit memory pages (%p %d) 0x%x", (void *) ((char *) memory + *size), new_size - *size, GetLastError ());
            memory = 0;
        }
    } else {
        memory = reserve_virtual_address_space (maximum_amount_of_pages);
        if (memory) {
            if (0 == commit_memory_pages ((char *) memory + maximum_allocation_size - new_size, new_size / Memory_Page)) {
                Error ("could not commit memory pages 0x%x", GetLastError ());
                release_virtual_address_space ((void *) memory);
                memory = 0;
            }
        } else {
            Error ("could not reserve memory 0x%x", GetLastError ());
        }
    }
    if (memory) {
        *size = new_size;
    } else {
        Error ("could not expand memory");
    }
    return (memory);
}

#define _Stack
#define Maximum_Amount_Of_Memory_Pages_For_Stack 1024

struct stack_prefix {
    int max_pages;
    unsigned char is_static;
    unsigned char terminator_size;
    unsigned short counter;
    int capacity;
    int occupied;
};

static inline struct stack_prefix *get_stack_prefix (_In_ void *_Stack stack) {
    return ((struct stack_prefix *) ((char *) stack - sizeof (struct stack_prefix)));
}

static inline int increment_stack (_In_ void *_Stack stack) {
    return (get_stack_prefix (stack)->counter += 1);
}

static inline int decrement_stack (_In_ void *_Stack stack) {
    return (get_stack_prefix (stack)->counter -= 1);
}

static inline int is_static_stack (_In_ _ROnly void *_Stack stack) {
    return (get_stack_prefix (stack)->is_static);
}

static inline int get_stack_size (_In_ _ROnly void *_Stack stack) {
    return (get_stack_prefix (stack)->occupied - sizeof (struct stack_prefix));
}

static inline int get_stack_terminator_size (_In_ _ROnly void *_Stack stack) {
    return (get_stack_prefix (stack)->terminator_size);
}

static inline int get_stack_remaining_capacity (_In_ _ROnly void *_Stack stack) {
    return (get_stack_prefix (stack)->capacity - get_stack_prefix (stack)->occupied);
}

static inline int is_stack_empty (_In_ _ROnly void *_Stack stack) {
    return (0 == get_stack_size (stack));
}

_Ret_notnull_ static inline void *get_stack_end (_In_ _ROnly void *_Stack stack) {
    return ((char *) stack + get_stack_size (stack));
}

static inline int is_pointer_from_stack (_In_ _ROnly void *ptr, _In_ _ROnly void *_Stack stack) {
    return (is_inside_range (ptr, stack, get_stack_end (stack)));
}

static inline int is_end_pointer_from_stack (_In_ _ROnly void *ptr, _In_ _ROnly void *_Stack stack) {
    return (is_end_inside_range (ptr, stack, get_stack_end (stack)));
}

_Ret_notnull_ static void *_Stack allocate_stack_ex (int terminator_size, int maximum_amount_of_memory_pages) {
    int cap;
    struct stack_prefix *prefix;

    Assert (terminator_size >= 0 && terminator_size <= 0xFF);
    cap = 0;
    prefix = (struct stack_prefix *) expand_memory (0, &cap, sizeof *prefix + terminator_size, maximum_amount_of_memory_pages);
    if (prefix) {
        memset (prefix, 0, sizeof *prefix + terminator_size);
        prefix->max_pages = maximum_amount_of_memory_pages;
        prefix->terminator_size = terminator_size;
        prefix->counter = 1;
        prefix->capacity = cap;
        prefix->occupied = sizeof *prefix;
    } else {
        Throw1 ("could not allocate memory for stack");
    }
    return (prefix + 1);
}

_Ret_notnull_ static inline void *_Stack allocate_stack (int terminator_size) {
    return (allocate_stack_ex (terminator_size, Maximum_Amount_Of_Memory_Pages_For_Stack));
}

static inline void *_Stack init_static_stack (_Out_ void *data, int data_size, int terminator_size) {
    struct stack_prefix *prefix;

    Assert (terminator_size >= 0 && terminator_size <= 0xFF);
    Assert (data_size > sizeof *prefix + terminator_size);
    memset (data, 0, sizeof *prefix + terminator_size);
    prefix = (struct stack_prefix *) data;
    prefix->capacity = data_size;
    prefix->occupied = sizeof *prefix;
    prefix->is_static = 1;
    prefix->terminator_size = terminator_size;
    prefix->counter = 1;
    return (prefix + 1);
}

static inline void terminate_stack (_In_ void *_Stack stack) {
    struct stack_prefix *prefix;

    prefix = get_stack_prefix (stack);
    memset ((char *) prefix + prefix->occupied, 0, prefix->terminator_size);
}

static void prepare_stack (_In_ void *_Stack stack, int tofit) {
    struct stack_prefix *prefix;

    prefix = get_stack_prefix (stack);
    if (prefix->capacity - prefix->occupied - prefix->terminator_size < tofit) {
        if (0 == is_static_stack (stack)) {
            void *_Stack mem;

            mem = expand_memory (prefix, &prefix->capacity, prefix->occupied + tofit + prefix->terminator_size, prefix->max_pages);
            if (mem) {
                prefix = (struct stack_prefix *) mem;
                Assert (prefix->capacity - prefix->occupied - prefix->terminator_size >= tofit);
            } else {
                Throw1 ("could not expand stack");
            }
        } else {
            Throw1 ("the capacity limit of static stack is reached");
        }
    }
}

static int prepare_stack_noexcept (_In_ void *_Stack stack, int tofit) {
    int result;
    struct stack_prefix *prefix;

    prefix = get_stack_prefix (stack);
    if (prefix->capacity - prefix->occupied - prefix->terminator_size < tofit) {
        if (0 == is_static_stack (stack)) {
            void *_Stack mem;

            mem = expand_memory (prefix, &prefix->capacity, prefix->occupied + tofit + prefix->terminator_size, prefix->max_pages);
            if (mem) {
                prefix = (struct stack_prefix *) mem;
                Assert (prefix->capacity - prefix->occupied - prefix->terminator_size >= tofit);
                result = 1;
            } else {
                result = 0;
            }
        } else {
            result = 0;
        }
    } else {
        result = 1;
    }
    return (result);
}

static inline void *_Stack acquire_stack (_In_opt_ void *_Stack stack) {
    if (stack) {
        increment_stack (stack);
    }
    return (stack);
}

static inline void release_stack_ (_In_opt_ void *_Stack stack) {
    if (stack && 0 == decrement_stack (stack) && !is_static_stack (stack)) {
        release_virtual_address_space (get_stack_prefix (stack));
    }
}

static inline void release_stack (_In_ void *_Stack *stack) {
    release_stack_ (*stack);
    *stack = 0;
}

static inline void clear_stack (_In_ void *_Stack stack) {
    get_stack_prefix (stack)->occupied = sizeof (struct stack_prefix);
    terminate_stack (stack);
}

static inline void pop_stack (_In_ void *_Stack stack, _In_range_ (>=, 0) int count) {
    int stack_size;

    Assert (stack);
    stack_size = get_stack_size (stack);
    count = Min (stack_size, count);
    if (count) {
        get_stack_prefix (stack)->occupied -= count;
        terminate_stack (stack);
    }
}

static inline void pop_stack_including (_In_ void *_Stack stack, _In_ _ROnly void *ptr) {
    Assert (stack);
    Assert (is_end_pointer_from_stack (ptr, stack));
    pop_stack (stack, to_offset (ptr, get_stack_end (stack)));
}

_Ret_notnull_ static inline void *push_stack_ (_In_ void *_Stack stack, int size) {
    void *mem;

    mem = get_stack_end (stack);
    get_stack_prefix (stack)->occupied += size;
    terminate_stack (stack);
    return (mem);
}

_Ret_notnull_ static inline void *push_stack (_In_ void *_Stack stack, int size) {
    prepare_stack (stack, size);
    return (push_stack_ (stack, size));
}

_Ret_maybenull_ static inline void *push_stack_noexcept (_In_ void *_Stack stack, int size) {
    return (prepare_stack_noexcept (stack, size) ? push_stack_ (stack, size) : 0);
}

_Ret_notnull_ static void *push_copy (_In_ void *_Stack stack, _In_ const void *ptr, int size) {
    void *result;

    result = push_stack (stack, size);
    memcpy (result, ptr, size);
    return (result);
}

static inline void *insert_stack (_In_ void *_Stack stack, _In_ _ROnly void *at, int size) {
    push_stack (stack, size);
    if ((char *) at + size < get_stack_end (stack)) {
        memmove ((char *) at + size, at, to_offset (at, get_stack_end (stack)) - size);
    }
    return (at);
}

static inline void erase_stack (_In_ void *_Stack stack, _In_ void *ptr, int size) {
    if (size > 0) {
        Assert (is_pointer_from_stack (ptr, stack));
        Assert (is_end_pointer_from_stack ((char *) ptr + size, stack));
        if ((char *) ptr + size == get_stack_end (stack)) {
            pop_stack (stack, size);
        } else {
            memmove (ptr, (char *) ptr + size, (char *) get_stack_end (stack) - ((char *) ptr + size));
            get_stack_prefix (stack)->occupied -= size;
            terminate_stack (stack);
        }
    }
}

static void *move_stack (_In_ void *_Stack stack, _In_ void *point, _In_ void *begin, int count) {
    if (count > 0 && point != begin) {
        Assert (!(point > begin && point < (char *) begin + count));
        insert_stack (stack, point, count);
        if (point < begin) {
            begin = (char *) begin + count;
        }
        memcpy (point, begin, count);
        erase_stack (stack, begin, count);
        if (point > begin) {
            point = (char *) point - count;
        }
    }
    return (point);
}

_Ret_notnull_ static void *push_stack_data_ex (_In_ void *_Stack stack, _In_reads_bytes_opt_ (data_size) const void *data, int data_size, _In_range_ (>, 0) int data_capacity, int do_zero) {
    char *ptr;

    ptr = (char *) push_stack (stack, data_capacity);
    data_size = Min (data_size, data_capacity);
    if (data && data_size) {
        memcpy (ptr, data, data_size);
    }
    if (do_zero) {
        memset (ptr + data_size, 0, data_capacity - data_size);
    }
    return (ptr);
}

_Ret_notnull_ static inline void *push_stack_data (_In_ void *_Stack stack, _In_reads_bytes_ (data_size) const void *data, int data_size) {
    return (push_stack_data_ex (stack, data, data_size, data_size, 0));
}

_Ret_notnull_ static void *insert_stack_data_ex (_In_ void *_Stack stack, _In_ void *at, _In_reads_bytes_opt_ (data_size) const void *data, int data_size, int data_capacity, int do_zero) {
    Assert (is_end_pointer_from_stack (at, stack));
    Assert (data_capacity);
    insert_stack (stack, at, data_capacity);
    data_size = Min (data_size, data_capacity);
    if (data && data_size) {
        memcpy (at, data, data_size);
    }
    if (do_zero && data_capacity - data_size) {
        memset ((char *) at + data_size, 0, data_capacity - data_size);
    }
    return (at);
}

_Ret_notnull_ static inline void *insert_stack_data (_In_ void *_Stack stack, _In_ void *at, _In_reads_bytes_ (data_size) const void *data, int data_size) {
    return (insert_stack_data_ex (stack, at, data, data_size, data_size, 0));
}

_Ret_notnull_ static void *append_stack_data_ex (_In_ void *_Stack stack, void *head, int head_size, _In_reads_bytes_opt_ (data_size) const void *data, int data_size, int data_capacity, int do_zero) {
    Assert (is_pointer_from_stack (head, stack));
    Assert (is_end_pointer_from_stack ((char *) head + head_size, stack));
    if ((char *) head + head_size == get_stack_end (stack)) {
        push_stack_data_ex (stack, data, data_size, data_capacity, do_zero);
    } else {
        insert_stack_data_ex (stack, (char *) head + head_size, data, data_size, data_capacity, do_zero);
    }
    return (head);
}

_Ret_notnull_ static inline void *append_stack_data (_In_ void *_Stack stack, _In_ void *head, int cover_size, _In_reads_bytes_ (data_size) const void *data, int data_size) {
    return (append_stack_data_ex (stack, head, cover_size, data, data_size, data_size, 0));
}

/* top-down stack */

#define _TDStack
#define Maximum_Amount_Of_Memory_Pages_For_TDStack 1024
#define TDStack_Maximum_Allocation_Size (Maximum_Amount_Of_Memory_Pages_For_TDStack * Memory_Page)

_Ret_notnull_ static inline unsigned long long *get_tdstack_values (_In_ const void *_TDStack stack) {
    return ((unsigned long long *) ((char *) stack + TDStack_Maximum_Allocation_Size - 3 * sizeof (unsigned long long)));
}

_Ret_notnull_ static inline int *get_tdstack_capacity_value (_In_ const void *_TDStack stack) {
    return ((int *) (get_tdstack_values (stack) + 1));
}

_Ret_notnull_ static inline int *get_tdstack_counter_value (_In_ const void *_TDStack stack) {
    return ((int *) (get_tdstack_values (stack) + 1) + 1);
}

_Ret_notnull_ static inline void **get_tdstack_top_pointer_value (_In_ const void *_TDStack stack) {
    return ((void **) (get_tdstack_values (stack) + 2));
}

static inline int get_tdstack_capacity (_In_ const void *_TDStack stack) {
    return (*get_tdstack_capacity_value (stack));
}

static inline int increment_tdstack (_In_ const void *_TDStack stack) {
    return (*get_tdstack_counter_value (stack) += 1);
}

static inline int decrement_tdstack (_In_ const void *_TDStack stack) {
    return (*get_tdstack_counter_value (stack) -= 1);
}

_Ret_notnull_ static inline void *get_tdstack_top (_In_ const void *_TDStack stack) {
    return (*get_tdstack_top_pointer_value (stack));
}

_Ret_notnull_ static inline void *get_tdstack_end (_In_ const void *_TDStack stack) {
    return (get_tdstack_values (stack));
}

static inline int get_tdstack_size (_In_ const void *_TDStack stack) {
    return ((char *) get_tdstack_end (stack) - (char *) get_tdstack_top (stack));
}

static inline int is_tdstack_pointer (_In_ const void *_TDStack stack, _In_ void *ptr) {
    return (ptr >= stack && ptr < get_tdstack_end (stack));
}

static inline void set_tdstack_top (_In_ const void *_TDStack stack, _In_ void *ptr) {
    if (get_tdstack_top (stack) <= ptr && get_tdstack_end (stack) >= ptr) {
        *get_tdstack_top_pointer_value (stack) = ptr;
    } else {
        Throw1 ("TDStack %s", get_tdstack_top (stack) <= ptr ? "overflow" : "underflow");
    }
}

static inline int get_tdstack_occupied_size (_In_ const void *_TDStack stack) {
    return ((char *) stack + TDStack_Maximum_Allocation_Size - (char *) get_tdstack_top (stack));
}

_Ret_notnull_ static const void *_TDStack allocate_tdstack (void) {
    const void *_TDStack stack;
    int cap;
    unsigned long long *values;

    cap = 0;
    stack = expand_topdown_memory (0, &cap, 3 * sizeof (unsigned long long), Maximum_Amount_Of_Memory_Pages_For_TDStack);
    if (0 == stack) Throw1 ("could not allocate top-down stack");
    values = get_tdstack_values (stack);
    values [0] = 0;
    values [1] = (unsigned long long) cap | (1llu << 32);
    values [2] = (unsigned long long) values;
    return (stack);
}

static inline const void *_TDStack acquire_tdstack (_In_ const void *_TDStack stack) {
    if (stack) {
        increment_tdstack (stack);
    }
    return (stack);
}

static inline void release_tdstack_ (_In_ const void *_TDStack stack) {
    if (stack && 0 >= decrement_tdstack (stack)) {
        release_virtual_address_space ((void *) stack);
    }
}

static inline void release_tdstack (_In_ const void *_TDStack *pstack) {
    if (*pstack && 0 >= decrement_tdstack (*pstack)) {
        release_virtual_address_space ((void *) *pstack);
        *pstack = 0;
    }
}

static inline void prepare_tdstack (_In_ const void *_TDStack stack, int tofit) {
    tofit += get_tdstack_occupied_size (stack);
    if (get_tdstack_capacity (stack) < tofit) {
        stack = expand_topdown_memory (stack, get_tdstack_capacity_value (stack), tofit, Maximum_Amount_Of_Memory_Pages_For_TDStack);
        if (0 == stack) Throw1 ("could not expand top-down stack");
    }
}

_Ret_notnull_ static inline void *push_tdstack (_In_ const void *_TDStack stack, int size) {
    prepare_tdstack (stack, size);
    return ((*(char **) get_tdstack_top_pointer_value (stack) -= size));
}

_Ret_notnull_ static inline void *insert_tdstack (_In_ const void *_TDStack stack, void *at, int size) {
    void *result, *top;

    top = get_tdstack_top (stack);
    if (at == top) {
        push_tdstack (stack, size);
    } else {
        memmove (push_tdstack (stack, size), top, (char *) at - (char *) top);
    }
    return ((char *) at - size);
}


