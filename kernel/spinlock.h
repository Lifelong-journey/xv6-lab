// Mutual exclusion lock.
struct spinlock {//确保多处理器对共享资源使用是互斥的
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

