# Makefile - 基于RK3588的嵌入式多终端通信系统
# 交叉编译：CROSS_COMPILE=aarch64-linux-gnu-

CC      ?= gcc
CROSS   ?=
CFLAGS  = -Wall -Wextra -O2 -I./include -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt -lsqlite3

# 源文件
COMMON_SRC    = src/common/protocol.c src/common/ipc.c
ROUTER_SRC    = src/router/router.c
PROTOCOL_SRC  = src/protocol_engine/protocol_engine.c
STORAGE_SRC   = src/storage/storage.c
MONITOR_SRC   = src/monitor/monitor.c
TERMINAL_SRC  = src/terminal/terminal.c

# 目标
TARGETS = router protocol_engine storage monitor terminal

.PHONY: all clean install

all: $(TARGETS)

router: $(COMMON_SRC) $(ROUTER_SRC)
	$(CROSS)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

protocol_engine: $(COMMON_SRC) $(PROTOCOL_SRC)
	$(CROSS)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

storage: $(COMMON_SRC) $(STORAGE_SRC)
	$(CROSS)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

monitor: $(COMMON_SRC) $(MONITOR_SRC)
	$(CROSS)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

terminal: $(COMMON_SRC) $(TERMINAL_SRC)
	$(CROSS)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

console:
	@echo "Console is Python-based, run: python3 src/console/console.py"

clean:
	rm -f $(TARGETS)

install: all
	mkdir -p /opt/rk3588-multiterminal/bin
	install -m 755 $(TARGETS) /opt/rk3588-multiterminal/bin/
	mkdir -p /opt/rk3588-multiterminal/sql
	install -m 644 sql/init.sql /opt/rk3588-multiterminal/sql/
	mkdir -p /opt/rk3588-multiterminal/config
	install -m 644 config/system.conf /opt/rk3588-multiterminal/config/

# 交叉编译（RK3588 aarch64）
cross:
	$(MAKE) CROSS=aarch64-linux-gnu-

# 运行数据库初始化
initdb:
	sqlite3 /tmp/rk3588_comm.db < sql/init.sql
	@echo "数据库已初始化: /tmp/rk3588_comm.db"
