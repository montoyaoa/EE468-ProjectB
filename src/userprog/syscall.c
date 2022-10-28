#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <debug.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
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
  case SYS_HALT:
    printf("SYS_HALT\n");
    break;
  //1
  case SYS_EXIT:
    printf("SYS_EXIT\n");
    fd = *((int*)f->esp + 1);
    break;
  //1
  case SYS_EXEC:
    printf("SYS_EXEC\n");
    break;
  //1
  case SYS_WAIT:
    printf("SYS_WAIT\n");
    break;
  //2
  case SYS_CREATE:
    printf("SYS_CREATE\n");
    break;
  //1
  case SYS_REMOVE:
    printf("SYS_REMOVE\n");
    break;
  //1
  case SYS_OPEN:
    printf("SYS_OPEN\n");
    break;
  //1
  case SYS_FILESIZE:
    printf("SYS_FILESIZE\n");
    fd = *((int*)f->esp + 1);
    break;
  //3
  case SYS_READ:
    printf("SYS_READ\n");
    break;
  case SYS_WRITE:
    debug_backtrace();
    printf("SYS_WRITE\n");
    //extract out the useful information from the stack and pass it
    //to write()
    fd = *((int*)f->esp + 1);
    void* buffer = (void*) (*((int*)f->esp + 2));
    unsigned size = *((unsigned*)f->esp + 3);
    hex_dump(f->esp, f->esp, 16, true);
    printf("in switch statment: fd=%d size=%ld\n", fd, size);
    f->eax = write(fd, buffer, size);
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

void halt (void){
}
void exit (int status){

}
//really a pid_t
int exec (const char *file){

}
int wait (pid_t){

}
bool create (const char *file, unsigned initial_size){

}
bool remove (const char *file){

}
int open (const char *file){

}
int filesize (int fd){

}
int read (int fd, void *buffer, unsigned length){

}
int write (int fd, const void *buffer, unsigned length){
  printf("called write() syscall. fd=%d length=%u", fd, length);
  //writing to console
  //if(fd == 1){
    putbuf(buffer, length);
  //}

}
void seek (int fd, unsigned position){

}
unsigned tell (int fd){

}
void close (int fd){

}
