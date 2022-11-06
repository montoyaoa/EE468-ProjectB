#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <debug.h>
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "list.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
void exitProcess(int status);
void* addrCheck(const void *vaddr);
void closeAllFiles(struct list* files);
struct proc_file* list_search(struct list* files, int fd);

struct proc_file 
{
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

struct lock file_lock;

void
syscall_init (void)
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int fd;
  int * p = f->esp;

  switch (*p)
  {
  //no arguments
  //no return value
  case SYS_HALT:
    //the HALT syscall immediately shuts down the OS
    shutdown_power_off();
    break;
  //1 argument: int status
  //no return value
  case SYS_EXIT:
  {
    addrCheck(p+1);
    exitProcess(*(p+1));
    break;
  }
  //1 argument: const char * cmd_line
  //return pid_t of newly run executable
  case SYS_EXEC:
  {
    addrCheck(p+1);
    addrCheck(*(p+1));
    f->eax = executeProcess(*(p+1));
    break;
  }
  //1 argument: pid_t child_pid
  //return int child exit status
  case SYS_WAIT:
  {
    addrCheck(p+1);
    f->eax = process_wait(*(p+1));
    break;
  }
  //2 arguments: const char * filename, unsigned initial_size
  //returns bool based on successful creation
  case SYS_CREATE:{
    const char * filename = (*((unsigned*)f->esp + 4));
    unsigned initial_size = *((unsigned*)f->esp + 5);

    if(filename == NULL)
    {
      exitProcess(-1);
    }

    lock_acquire(&file_lock);
    f->eax = filesys_create(filename,initial_size);
    lock_release(&file_lock);
    break;
  }
  //1 argument: const char * filename
  //returns bool based on successful deletion
  case SYS_REMOVE:
    addrCheck(p+1);
    addrCheck(*(p+1));
    lock_acquire(&file_lock);
    if (filesys_remove(*(p+1)) == NULL) {
      f->eax = false;
    } else {
      f->eax = true;
    }
    lock_release(&file_lock);
    break;
  //1 argument: const char * filename
  //returns int file descriptor
  case SYS_OPEN:
    addrCheck(p+1);
    addrCheck(*(p+1));
    lock_acquire(&file_lock);
    struct file* fptr = filesys_open (*(p+1));
    lock_release(&file_lock);
    if(fptr == NULL) {
      f->eax = -1;
    }
    else {
      struct proc_file *pfile = malloc(sizeof(*pfile));
      pfile->ptr = fptr;
      pfile->fd = thread_current()->fdCount;
      thread_current()->fdCount++;
      list_push_back(&thread_current()->files, &pfile->elem);
      f->eax = pfile->fd;
    }
    break;
  //1 argument: int filedescriptor
  //returns int size of file in bytes
  case SYS_FILESIZE:
    fd = *((int*)f->esp + 5);
    lock_acquire(&file_lock);
    f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
    lock_release(&file_lock);
    break;
  //3 arguments: int filedescriptor, const void * buffer,
  case SYS_READ:
    addrCheck(p+7);
    addrCheck(*(p+6));
    if (*(p+5) == 0) {
      int i;
      uint8_t* buffer = *(p+6);
      for (i = 0; i < *(p+7); i++) {
        buffer[i] = input_getc();
      }
      f->eax = *(p+7);
    } else {
      struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
      if (fptr == NULL) {
        f->eax = -1;
      } else {
        lock_acquire(&file_lock);
        f->eax = file_read(fptr->ptr, *(p+6), *(p+7));
        lock_release(&file_lock);
      }
    }
    break;
  case SYS_WRITE:
    if(*(p+5)==1){
			putbuf(*(p+6),*(p+7));
			f->eax = *(p+7);
		}
    else{
			struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				lock_acquire(&file_lock);
				f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
				lock_release(&file_lock);
			}
		}
    break;
  //2
  case SYS_SEEK:
    fd = *((int*)f->esp + 1);
    lock_acquire(&file_lock);
    file_seek(list_search(&thread_current()->files, *(p+4))->ptr, *(p+5));
    lock_release(&file_lock);
    break;
  //1
  case SYS_TELL:
    addrCheck(p+1);
    lock_acquire(&file_lock);
    f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
    lock_release(&file_lock);
    break;
  //1
  case SYS_CLOSE:
    addrCheck(p+1);
    lock_acquire(&file_lock);
    close_file(&thread_current()->files, *(p+1));
    lock_release(&file_lock);
    break;

  default:
    printf("ERROR\n");
    break;
  }
}

void exitProcess(int status) {
  struct list_elem *x;
  for (x = list_begin(&thread_current()->parent->childProcess); x != list_end(&thread_current()->parent->childProcess); x = list_next(x)) {
    struct child *y = list_entry(x, struct child, elem);
    if (y->tid == thread_current()->tid){
      y->used = true;
      y->exit_error = status;
    }
  }
  thread_current()->exit_error = status;
  if (thread_current()->parent->wait == thread_current()->tid) {
    sema_up(&thread_current()->parent->childLock);
  }
  thread_exit();
}

void* addrCheck(const void *vaddr) {
  if (is_user_vaddr(vaddr) == 0) {
    exitProcess(-1);
    return 0;
  }
  void *tmp = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!tmp) {
    exitProcess(-1);
    return 0;
  }
  return tmp;
}

void closeAllFiles(struct list* files) {
  struct list_elem *x;
  while(!list_empty(files)){
    x = list_pop_front(files);
    struct proc_file *f = list_entry(x, struct proc_file, elem);
    file_close(f->ptr);
    list_remove(x);
    free(f);
  }
}

struct proc_file* list_search(struct list* files, int fd){
	struct list_elem *e;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
    struct proc_file *f = list_entry (e, struct proc_file, elem);
    if(f->fd == fd)
      return f;
  }
  return NULL;
}
int executeProcess(char *file_name){
	lock_acquire(&file_lock);
	char * fn = malloc (strlen(file_name)+1);
	strlcpy(fn, file_name, strlen(file_name)+1);

	char * save_ptr;
	fn = strtok_r(fn," ",&save_ptr);

	struct file* f = filesys_open (fn);

	if(f==NULL){
	  lock_release(&file_lock);
	  return -1;
	}
	else{
	  file_close(f);
	  lock_release(&file_lock);
	  return process_execute(file_name);
	}
}

void close_file(struct list* files, int fd) {
  struct list_elem *e;
  struct proc_file *f;
  for (e = list_begin(files); e != list_end(files); e = list_next(e)){
    f = list_entry(e, struct proc_file, elem);
    if (f->fd == fd){
      file_close(f->ptr);
      list_remove(e);
    }
  }
  free(f);
}
