# Extended Attributes, ACLs, and Inotify

This document covers advanced file attributes, fine-grained access control, and file event monitoring, based on *The Linux Programming Interface (TLPI)* Chapters 16, 17, and 19.

---

## 1. Extended Attributes (Chapter 16)

Standard UNIX file attributes (like ownership and permissions) are rigidly defined by the `inode` structure. **Extended Attributes (EAs)** allow users and applications to associate arbitrary metadata with a file or directory in the form of `name:value` pairs.

### Namespaces
EA names must fall into one of four namespaces:
- `user`: For arbitrary application use (e.g., `user.mime_type: "text/html"`).
- `trusted`: Similar to `user`, but accessible only to privileged processes.
- `system`: Used by the kernel for system objects (e.g., ACLs are stored here).
- `security`: Used by security modules like SELinux.

### The EA API
The `<sys/xattr.h>` header provides the API to manipulate Extended Attributes:
- `setxattr(path, name, value, size, flags)`: Creates or modifies an EA.
- `getxattr(path, name, value, size)`: Retrieves the value of an EA.
- `listxattr(path, list, size)`: Returns a list of all EA names associated with the file.
- `removexattr(path, name)`: Deletes an EA.

---

## 2. Access Control Lists - ACLs (Chapter 17)

The traditional UNIX permission model (Owner, Group, Others) is sometimes too restrictive. For instance, you cannot give "User A" read access and "User B" write access without creating a specific group for them.

**Access Control Lists (ACLs)** solve this by allowing you to attach a list of fine-grained permissions to a file. (Under the hood, ACLs are stored as Extended Attributes in the `system` namespace).

### Using ACLs
From the command line, ACLs are managed using:
- `setfacl -m u:alice:rw file.txt`: Modifies the ACL to give user 'alice' read/write access.
- `getfacl file.txt`: Displays the current ACLs of the file.

When a file has an ACL, the output of `ls -l` will show a `+` sign at the end of the permission string (e.g., `-rw-rw-r--+`).

### ACL API
While command-line tools are preferred, programs can manipulate ACLs programmatically using the `<sys/acl.h>` header and linking against `-lacl`. 

> [!WARNING]
> Programmatic ACL manipulation in C is extremely complex due to the opaque structures involved. In most embedded and system applications, calling `system("setfacl ...")` or using higher-level abstractions is often preferred unless performance is hyper-critical.

---

## 3. Monitoring File Events: Inotify (Chapter 19)

Historically, if an application wanted to know when a file changed (e.g., a text editor checking if another program modified an open file), it had to periodically poll the file system using `stat()`. This was inefficient and consumed heavy CPU/battery resources.

Linux introduced the **`inotify`** API to solve this elegantly. It provides a mechanism for monitoring file system events asynchronously.

### How Inotify Works

1. **Initialize an Instance:**
   ```c
   #include <sys/inotify.h>
   int fd = inotify_init();
   ```
   This creates an `inotify` instance and returns a standard file descriptor.

2. **Add Watches:**
   ```c
   int wd = inotify_add_watch(fd, "/path/to/watch", IN_MODIFY | IN_CREATE | IN_DELETE);
   ```
   You tell the kernel which file or directory to watch, and exactly which events you care about. A single `inotify` descriptor can watch thousands of different directories.

3. **Read Events:**
   When an event occurs (e.g., a file is deleted), the kernel makes the data available on the file descriptor. You simply `read()` from `fd` into a buffer, which populates one or more `struct inotify_event` structures.
   ```c
   struct inotify_event {
       int      wd;       /* Watch descriptor */
       uint32_t mask;     /* Mask describing event (e.g., IN_DELETE) */
       uint32_t cookie;   /* Unique cookie associating related events (rename) */
       uint32_t len;      /* Size of name field */
       char     name[];   /* Optional null-terminated name */
   };
   ```

### Best Practices & Limitations
- **Recursive Watching:** `inotify` does **not** watch directories recursively. If you watch `/home`, you will not get events for `/home/user/file.txt`. You must manually add a watch for every single subdirectory.
- **Event Queues:** If your application processes events too slowly, the kernel's `inotify` event queue will fill up, and events will be dropped (yielding an `IN_Q_OVERFLOW` event).
- **Blocking Read:** The `read()` call on the inotify descriptor blocks until an event occurs. You can use `select()` or `poll()` if you need to monitor other inputs simultaneously.
