#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <debug.h>
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);

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
  printf ("system call!\n");
  printf("syscall type: ");
  int fd;

  switch (*(int*)f->esp)
  {
  //no arguments
  //no return value
  case SYS_HALT:
    printf("SYS_HALT\n");
    //the HALT syscall immediately shuts down the OS
    shutdown_power_off();
    break;
  //1 argument: int status
  //no return value
  case SYS_EXIT:
    printf("SYS_EXIT\n");
    fd = *((int*)f->esp + 1);
    break;
  //1 argument: const char * cmd_line
  //return pid_t of newly run executable
  case SYS_EXEC:
    printf("SYS_EXEC\n");
    break;
  //1 argument: pid_t child_pid
  //return int child exit status
  case SYS_WAIT:
    printf("SYS_WAIT\n");
    break;
  //2 arguments: const char * filename, unsigned initial_size
  //returns bool based on successful creation
  case SYS_CREATE:
    printf("SYS_CREATE\n");
    //fd = *((int*)f->esp + 2);
    //void* buffer = (void*) (*((int*)f->esp + 6));
    //unsigned size = *((unsigned*)f->esp + 3);
    //printf("fd=%d &fd=%x\n", fd, f->esp + 2);
    //printf("buffer addr=%x\n", buffer);
    //printf("size=%x &size=%x\n", size, f->esp + 12);
    hex_dump(f->esp, f->esp, 100, true);
    //printf("in switch statment: fd=%d size=%ld\n", fd, size);
    //f->eax = write(fd, buffer, size);
    lock_acquire(&file_lock);
    //f->eax = filesys_create(name,size);
    lock_release(&file_lock);
    break;
  //1 argument: const char * filename
  //returns bool based on successful deletion
  case SYS_REMOVE:
    printf("SYS_REMOVE\n");
    break;
  //1 argument: const char * filename
  //returns int file descriptor
  case SYS_OPEN:
    printf("SYS_OPEN\n");
    break;
  //1 argument: int filedescriptor
  //returns int size of file in bytes
  case SYS_FILESIZE:
    printf("SYS_FILESIZE\n");
    fd = *((int*)f->esp + 5);
    break;
  //3 arguments: int filedescriptor, const void * buffer, 
  case SYS_READ:
    printf("SYS_READ\n");
    break;
  case SYS_WRITE:
    printf("SYS_WRITE\n");
    //extract out the useful information from the stack and pass it
    //to write()
    fd = *((int*)f->esp + 5);
    void* buffer = (void*) (*((int*)f->esp + 6));
    unsigned size = *((unsigned*)f->esp + 7);
    //printf("fd=%d &fd=%x\n", fd, f->esp + 2);
    //printf("buffer addr=%x\n", buffer);
    //printf("size=%x &size=%x\n", size, f->esp + 12);
    //hex_dump(f->esp, f->esp, 100, true);
    //printf("in switch statment: fd=%d size=%ld\n", fd, size);
    //f->eax = write(fd, buffer, size);

    //writing to console
    if(fd == 1){
      //use putbuf() to write to the console
      putbuf(buffer, size);
      //return the number of bytes written
      f->eax = size;
    }
    break;
  //2
  case SYS_SEEK:
    printf("SYS_SEEK\n");
    fd = *((int*)f->esp + 1);
    break;
  //1
  case SYS_TELL:
    printf("SYS_TELL\n");
    fd = *((int*)f->esp + 1);
    break;
  //1
  case SYS_CLOSE:
    printf("SYS_CLOSE\n");
    fd = *((int*)f->esp + 1);
    break;
  default:
    printf("ERROR\n");
    break;
  }
  thread_exit ();
}