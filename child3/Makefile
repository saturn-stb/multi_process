# ---------------------------------------------------------
# 모든 하위 소스 탐색 및 .obj 폴더에 오브젝트 관리
# ---------------------------------------------------------
CC ?= gcc
CFLAGS += -Wall -Wextra -pthread -MMD -MP # -MMD: .d 파일 생성, -MP: 헤더 삭제 시 에러 방지

ifneq ($(findstring arm,$(CC)),)
CFLAGS += -march=armv7-a -mfpu=neon  # ARM 전용 최적화 옵션 예시
endif

LDFLAGS +=
LDLIBS  += -lrt

CUR_DIR := $(CURDIR)

TARGET = taskChild3
OBJ_DIR = .obj

SRCS = $(shell find . -name "*.c")
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
# 1. 생성된 모든 .d 파일 목록 정의
DEPS = $(SRCS:%.c=$(OBJ_DIR)/%.d)

# include 디렉토리 목록을 정의합니다.
INC_DIRS = ../include inc

INC_DIRS += $(shell find . -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

FINAL_TARGET = $(if $(OUTPUT_DIR), $(OUTPUT_DIR)/$(TARGET), ../$(TARGET))

all: $(FINAL_TARGET)

$(FINAL_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@
	@echo "[Done] $(TARGET) build complete."

$(OBJ_DIR)/%.o: %.c
	@echo "========================================"
	@echo "  Building Process: [ $(TARGET) ]"
	@echo "  Source Files: $(SRCS)"
	@echo "  Output Path: $@"
	@echo "========================================"
	@mkdir -p $(dir $@)
	@echo "  Compiling: $< -> $@"
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

# 2. 생성된 의존성 파일들을 Makefile에 포함
# 파일이 없어도 에러가 나지 않도록 '-'를 붙입니다.
-include $(DEPS)

clean:
	@echo "----------------------------------------"
	@echo "  Cleaning in: [ $(CUR_DIR) ]"  # 현재 작업 중인 절대 경로 출력
	@echo "  Cleaning Process: [ $(TARGET) ]"
	@echo "  Removing: $(OBJ_DIR)/"
	@echo "  Removing: $(FINAL_TARGET)"
	@echo "----------------------------------------"
	@rm -rf $(OBJ_DIR)
	@rm -f $(FINAL_TARGET)
	@echo "[Clean] $(TARGET) artifacts removed."

.PHONY: all clean
