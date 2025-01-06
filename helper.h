static void pinThread(int cpu) {
  ::cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
    std::perror("Unable to pin thread to selected CPU core");
    std::exit(EXIT_FAILURE);
  }
}
