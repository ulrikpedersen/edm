#include <stdio.h>
#include <stdlib.h>

#include "thread_priv.h"

static int g_init = 1;
static cma_t_attr g_master_mu_attr;
static cma_t_mutex g_master_mutex;
static cma_t_attr g_master_cv_attr;
static cma_t_cond g_master_cv;

static void do_init ( void ) {

cma_t_attr attr;

  attr = cma_c_null;

  cma_attr_create( &g_master_mu_attr, &attr );
  cma_mutex_create( &g_master_mutex, &g_master_mu_attr );

  cma_attr_create( &g_master_cv_attr, &attr );
  cma_cond_create( &g_master_cv, &g_master_cv_attr );

}

int thread_init( void ) {

  if ( g_init ) {
    g_init = 0;
    cma_init();
    do_init();
  }

  return THR_SUCCESS;

}

int thread_create_lock_handle (
  THREAD_LOCK_HANDLE *handle
) {

THREAD_LOCK_PTR priv_thr_lock;
cma_t_attr attr;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock =
   (THREAD_LOCK_PTR) calloc( 1, sizeof(THREAD_LOCK_TYPE) );
  if ( !priv_thr_lock ) return THR_NOMEM;

  attr = cma_c_null;

  cma_attr_create( &priv_thr_lock->mu_attr, &attr );

  cma_mutex_create( &priv_thr_lock->mutex,
   &priv_thr_lock->mu_attr );

  *handle = (THREAD_LOCK_HANDLE) priv_thr_lock;

  return THR_SUCCESS;

}

int thread_lock (
  THREAD_LOCK_HANDLE handle
) {

THREAD_LOCK_PTR priv_thr_lock;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock = (THREAD_LOCK_PTR) handle;
  if ( !priv_thr_lock ) return THR_BADPARAM;

  cma_mutex_lock( &priv_thr_lock->mutex );

  return THR_SUCCESS;

}

int thread_unlock (
  THREAD_LOCK_HANDLE handle
) {

THREAD_LOCK_PTR priv_thr_lock;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock = (THREAD_LOCK_PTR) handle;
  if ( !priv_thr_lock ) return THR_BADPARAM;

  cma_mutex_unlock( &priv_thr_lock->mutex );

  return THR_SUCCESS;

}

int thread_create_lock_array_handle (
  THREAD_LOCK_ARRAY_HANDLE *handle,
  int num_locks
) {

THREAD_LOCK_ARRAY_PTR priv_thr_lock_array;
cma_t_attr attr;
int i;

  if ( g_init ) return THR_BADSTATE;

  if ( num_locks < 1 ) return THR_BADPARAM;
  if ( num_locks > 256 ) return THR_BADPARAM;

  priv_thr_lock_array =
   (THREAD_LOCK_ARRAY_PTR) calloc( 1, sizeof(THREAD_LOCK_ARRAY_TYPE) );
  if ( !priv_thr_lock_array ) return THR_NOMEM;

  priv_thr_lock_array->num_elements = num_locks;

  priv_thr_lock_array->mu_attr_array = (cma_t_attr *) calloc( num_locks,
   sizeof(cma_t_attr) );
  if ( !priv_thr_lock_array->mu_attr_array ) return THR_NOMEM;

  priv_thr_lock_array->mutex_array = (cma_t_attr *) calloc( num_locks,
   sizeof(cma_t_mutex) );
  if ( !priv_thr_lock_array->mutex_array ) return THR_NOMEM;

  attr = cma_c_null;

  for ( i=0; i<num_locks; i++ ) {

    cma_attr_create( &priv_thr_lock_array->mu_attr_array[i], &attr );

    cma_mutex_create( &priv_thr_lock_array->mutex_array[i],
     &priv_thr_lock_array->mu_attr_array[i] );

  }

  *handle = (THREAD_LOCK_ARRAY_HANDLE) priv_thr_lock_array;

  return THR_SUCCESS;

}

int thread_lock_array_element (
  THREAD_LOCK_ARRAY_HANDLE handle,
  int element
) {

THREAD_LOCK_ARRAY_PTR priv_thr_lock_array;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock_array = (THREAD_LOCK_ARRAY_PTR) handle;
  if ( !priv_thr_lock_array ) return THR_BADPARAM;

  if ( ( element < 0 ) || ( element >= priv_thr_lock_array->num_elements ) )
    return THR_BADPARAM;

  cma_mutex_lock( &priv_thr_lock_array->mutex_array[element] );

  return THR_SUCCESS;

}

int thread_unlock_array_element (
  THREAD_LOCK_ARRAY_HANDLE handle,
  int element
) {

THREAD_LOCK_ARRAY_PTR priv_thr_lock_array;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock_array = (THREAD_LOCK_ARRAY_PTR) handle;
  if ( !priv_thr_lock_array ) return THR_BADPARAM;

  if ( ( element < 0 ) || ( element >= priv_thr_lock_array->num_elements ) )
    return THR_BADPARAM;

  cma_mutex_unlock( &priv_thr_lock_array->mutex_array[element] );

  return THR_SUCCESS;

}

int thread_create_handle (
  THREAD_HANDLE *handle,
  void *app_data
) {

THREAD_ID_PTR priv_handle;
int ret_stat;
cma_t_attr attr;

  *handle = (THREAD_HANDLE) NULL;

  priv_handle = (THREAD_ID_PTR) calloc( 1, sizeof(THREAD_ID_TYPE) );
  if ( !priv_handle ) {
    ret_stat = THR_NOMEM;
    goto err_return;
  }

  priv_handle->process_active = 0;

  if ( g_init ) {
    g_init = 0;
    cma_init();
    do_init();
  }

  memcpy( &priv_handle->master_mu_attr, &g_master_mu_attr,
   sizeof(cma_t_attr) );
  memcpy( &priv_handle->master_mutex, &g_master_mutex,
   sizeof(cma_t_mutex) );
  memcpy( &priv_handle->master_cv_attr, &g_master_cv_attr,
   sizeof(cma_t_attr) );
  memcpy( &priv_handle->master_cv, &g_master_cv,
   sizeof(cma_t_cond) );

  attr = cma_c_null;

  cma_attr_create( &priv_handle->mu_attr, &attr );
  cma_mutex_create( &priv_handle->mutex, &priv_handle->mu_attr );

  cma_attr_create( &priv_handle->cv_attr, &attr );
  cma_cond_create( &priv_handle->cv, &priv_handle->cv_attr );

  cma_attr_create( &priv_handle->thr_attr, &attr );

  priv_handle->application_data = app_data;

  *handle = (THREAD_HANDLE) priv_handle;

  return THR_SUCCESS;

err_return:
  return ret_stat;

}

int thread_destroy_handle (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;
int ret_stat;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    ret_stat = THR_BADPARAM;
    goto err_return;
  }

  cma_attr_delete( &priv_handle->mu_attr );
  cma_mutex_delete( &priv_handle->mutex );

  cma_attr_delete( &priv_handle->cv_attr );
  cma_cond_delete( &priv_handle->cv );

  cma_attr_delete( &priv_handle->thr_attr );

  if ( priv_handle->process_active ) {
    cma_thread_detach( &priv_handle->os_thread_id );
  }

  free( priv_handle );
  priv_handle = NULL;

  return THR_SUCCESS;

err_return:
  return ret_stat;

}

int thread_destroy_lock_handle (
  THREAD_LOCK_HANDLE handle
) {

THREAD_LOCK_PTR priv_thr_lock;

  if ( g_init ) return THR_BADSTATE;

  priv_thr_lock = (THREAD_LOCK_PTR) handle;
  if ( !priv_thr_lock ) return THR_BADPARAM;

  cma_attr_delete( &priv_thr_lock->mu_attr );
  cma_mutex_delete( &priv_thr_lock->mutex );

  free( priv_thr_lock );
  priv_thr_lock = NULL;

  return THR_SUCCESS;

}

void *thread_get_app_data (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return (void *) NULL;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return (void *) NULL;

  return priv_handle->application_data;

}

int thread_set_stack (
  THREAD_HANDLE handle,
  int size ) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    return THR_BADPARAM;
  }

  cma_attr_set_stacksize( &priv_handle->thr_attr,
   (long unsigned int) size );

  return THR_SUCCESS;

}

int thread_get_stack (
  THREAD_HANDLE handle,
  int *size ) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    return THR_BADPARAM;
  }

  cma_attr_get_stacksize( &priv_handle->thr_attr,
   (long unsigned int *) size );

  return THR_SUCCESS;

}

int thread_set_stack_size (
  THREAD_HANDLE handle,
  int size ) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    return THR_BADPARAM;
  }

  cma_attr_set_stacksize( &priv_handle->thr_attr,
   (long unsigned int) size );

  return THR_SUCCESS;

}

int thread_get_stack_size (
  THREAD_HANDLE handle,
  int *size ) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    return THR_BADPARAM;
  }

  cma_attr_get_stacksize( &priv_handle->thr_attr,
   (long unsigned int *) size );

  return THR_SUCCESS;

}

int thread_create_proc (
  THREAD_HANDLE handle,
  void (*proc)( void * )
) {

THREAD_ID_PTR priv_handle;
int ret_stat;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    ret_stat = THR_BADPARAM;
    goto err_return;
  }

  cma_thread_create( &priv_handle->os_thread_id, &priv_handle->thr_attr,
   (cma_t_start_routine) proc, handle );

  priv_handle->process_active = 1;

  return THR_SUCCESS;

err_return:
  return ret_stat;

}

int thread_set_proc_priority (
  THREAD_HANDLE handle,
  char *priority	/* "h","m","l" */
) {

THREAD_ID_PTR priv_handle;
cma_t_priority prior;
int ret_stat;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) {
    ret_stat = THR_BADPARAM;
    goto err_return;
  }

  switch ( priority[0] ) {

    case 'h':
    case 'H':
      prior = cma_c_prio_through_max;
      break;

    case 'm':
    case 'M':
      prior = cma_c_prio_through_mid;
      break;

    case 'l':
    case 'L':
      prior = cma_c_prio_through_min;
      break;

    default:
      ret_stat = THR_BADPARAM;
      goto err_return;

  }

  cma_thread_set_priority( &priv_handle->os_thread_id, prior );

  return THR_SUCCESS;

err_return:
  return ret_stat;

}

int thread_wait_til_complete (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;
int result;
cma_t_exit_status stat;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_thread_join( &priv_handle->os_thread_id, &stat, NULL );

  priv_handle->process_active = 0;

  return (int) stat;

}

int thread_lock_master (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_lock( &priv_handle->master_mutex );

  return THR_SUCCESS;

}

int thread_unlock_master (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_unlock( &priv_handle->master_mutex );

  return THR_SUCCESS;

}

int thread_lock_global ( void ) {

  if ( g_init ) return THR_BADSTATE;

  cma_mutex_lock( &g_master_mutex );

  return THR_SUCCESS;

}

int thread_unlock_global ( void ) {

  if ( g_init ) return THR_BADSTATE;

  cma_mutex_unlock( &g_master_mutex );

  return THR_SUCCESS;

}

int thread_wait_for_signal (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_lock( &priv_handle->mutex );
  cma_cond_wait( &priv_handle->cv, &priv_handle->mutex );
  cma_mutex_unlock( &priv_handle->mutex );

  return THR_SUCCESS;

}

int thread_timed_wait_for_signal (
  THREAD_HANDLE handle,
  double seconds
) {

THREAD_ID_PTR priv_handle;
cma_t_status stat;
struct timeval timer_exp_time;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_lock( &priv_handle->mutex );

  cma_time_get_expiration( &timer_exp_time,
   (cma_t_interval) seconds );

  stat = cma_cond_timed_wait( &priv_handle->cv, &priv_handle->mutex,
   &timer_exp_time );

  cma_mutex_unlock( &priv_handle->mutex );

  if ( stat == cma_s_timed_out ) return THR_TIMEOUT;

  return THR_SUCCESS;

}

int thread_signal (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_cond_signal( &priv_handle->cv );

  return THR_SUCCESS;

}

int thread_signal_from_ast (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_cond_signal_int( &priv_handle->cv );

  return THR_SUCCESS;

}

int thread_delay (
  THREAD_HANDLE handle,
  double seconds
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_lock( &priv_handle->mutex );

  cma_time_get_expiration( &priv_handle->timer_exp_time,
   (cma_t_interval) seconds );

  cma_cond_timed_wait( &priv_handle->cv, &priv_handle->mutex,
   &priv_handle->timer_exp_time );

  cma_mutex_unlock( &priv_handle->mutex );

  return THR_SUCCESS;

}

int thread_init_timer (
  THREAD_HANDLE handle,
  double seconds
) {

THREAD_ID_PTR priv_handle;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  priv_handle->timer_tick = seconds;
  priv_handle->delta_time.tv_sec = (int) seconds;
  priv_handle->delta_time.tv_usec =
   (int) ( ( seconds - (float) priv_handle->delta_time.tv_sec ) *
   1.0e6 );

  cma_time_get_expiration( &priv_handle->timer_exp_time,
   (cma_t_interval) seconds );

  return THR_SUCCESS;

}

int thread_wait_for_timer (
  THREAD_HANDLE handle
) {

THREAD_ID_PTR priv_handle;
double sec, sec1, sec2;

  if ( g_init ) return THR_BADSTATE;

  priv_handle = (THREAD_ID_PTR) handle;
  if ( !priv_handle ) return THR_BADPARAM;

  cma_mutex_lock( &priv_handle->mutex );

  cma_cond_timed_wait( &priv_handle->cv, &priv_handle->mutex,
   &priv_handle->timer_exp_time );

  sec1 = (double) priv_handle->timer_exp_time.tv_sec +
   ( (double) priv_handle->timer_exp_time.tv_usec ) / 1.0e6;

  sec2 = (double) priv_handle->delta_time.tv_sec +
   ( (double) priv_handle->delta_time.tv_usec ) / 1.0e6;

  sec = sec1 + sec2;

  priv_handle->timer_exp_time.tv_sec = (int) sec;
  priv_handle->timer_exp_time.tv_usec =
   (int) ( ( sec - (double) priv_handle->timer_exp_time.tv_sec )
   * 1.0e6 );

  cma_mutex_unlock( &priv_handle->mutex );

  return THR_SUCCESS;

}
