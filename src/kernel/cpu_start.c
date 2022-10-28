extern void __arm64_cpu_start(void);

int cpu_start(void) {
	__arm64_cpu_start();

	return KR_OK;
}

