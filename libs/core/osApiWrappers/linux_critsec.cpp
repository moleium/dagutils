#include <osApiWrappers/critsec.h>

#include <pthread.h>

struct pthread_mutex_wrapper {
  pthread_mutex_t mutex;
  char padding[40 - sizeof(pthread_mutex_t)];
};

void create_critical_section(void *cs, const char * /*name*/) {
  pthread_mutex_wrapper *wrapper = (pthread_mutex_wrapper *)cs;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&wrapper->mutex, &attr);
  pthread_mutexattr_destroy(&attr);
}

void delete_critical_section(void *cs) {
  pthread_mutex_destroy(&((pthread_mutex_wrapper *)cs)->mutex);
}

void enter_critical_section(void *cs, const char * /*name*/) {
  pthread_mutex_lock(&((pthread_mutex_wrapper *)cs)->mutex);
}

bool try_enter_critical_section(void *cs) {
  return (pthread_mutex_trylock(&((pthread_mutex_wrapper *)cs)->mutex) == 0);
}

void leave_critical_section(void *cs) {
  pthread_mutex_unlock(&((pthread_mutex_wrapper *)cs)->mutex);
}
