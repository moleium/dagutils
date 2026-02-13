#include <osApiWrappers/dag_stackHlp.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#if !defined(__EMSCRIPTEN__) && defined(__GLIBC__)
#include <execinfo.h>
#endif

inline int clamp0(int a) { return a > 0 ? a : 0; }

unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int skip_frames) {
#if defined(__EMSCRIPTEN__) || !defined(__GLIBC__)
  (void)stack;
  (void)max_size;
  (void)skip_frames;
  return 0;
#else
  std::vector<void *> temp_stack(max_size + skip_frames + 1);
  int nframes = backtrace(temp_stack.data(), temp_stack.size());

  nframes = clamp0(nframes - (skip_frames + 1));

  if ((unsigned)nframes > max_size)
    nframes = max_size;

  memcpy(stack, temp_stack.data() + skip_frames + 1, nframes * sizeof(void *));

  if ((unsigned)nframes < max_size)
    stack[nframes] = (void *)(~uintptr_t(0));

  return nframes;
#endif
}

unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size,
                                   const void * /*ctx_ptr*/) {
  if (max_size)
    stack[0] = (void *)(~uintptr_t(0));
  return 0;
}

const char *stackhlp_get_call_stack(char *out_buf, int out_buf_sz,
                                    const void *const *stack,
                                    unsigned max_size) {
#if defined(__EMSCRIPTEN__) || !defined(__GLIBC__)
  if (out_buf && out_buf_sz > 0)
    *out_buf = '\0';
  (void)stack;
  (void)max_size;
#else
  char **symbols =
      backtrace_symbols(const_cast<void *const *>(stack), max_size);
  if (out_buf && out_buf_sz > 0)
    *out_buf = '\0';

  if (symbols && out_buf && out_buf_sz > 0) {
    for (unsigned i = 0; i < max_size && stack[i] != (void *)(~uintptr_t(0));
         ++i) {
      if (strlen(out_buf) + strlen(symbols[i]) + 2 > (size_t)out_buf_sz)
        break;
      strncat(out_buf, symbols[i], out_buf_sz - strlen(out_buf) - 1);
      strncat(out_buf, "\n", out_buf_sz - strlen(out_buf) - 1);
    }
    free(symbols);
  }
#endif
  return out_buf;
}

void *stackhlp_get_bp() { return __builtin_frame_address(0); }

namespace stackhelp {
namespace ext {
CallStackResolverCallbackAndSizePair
capture_extended_call_stack(stackhelp::CallStackInfo /*stack*/) {
  return {};
}
CallStackContext
set_extended_call_stack_capture_context(CallStackContext /*new_context*/) {
  return {};
}
} // namespace ext
} // namespace stackhelp
