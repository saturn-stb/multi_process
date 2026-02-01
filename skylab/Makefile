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

TARGET = SKYLAB
OBJ_DIR = .obj
SRC_DIR = ./src
COMMON_DIR = ../common/src

# 1. 소스 파일 목록 (현재 폴더 및 common 폴더의 .c 포함)
# 현재 디렉토리와 common 디렉토리의 .c 파일을 각각 찾습니다.
SRCS_LOCAL = $(wildcard $(SRC_DIR)/*.c)
SRCS_COMMON = $(wildcard $(COMMON_DIR)/*.c)
SRCS = $(SRCS_LOCAL) $(SRCS_COMMON)

# 2. 오브젝트 파일 목록 (경로 정보를 제거하고 .obj 폴더 내에 생성되도록 설정)
# notdir를 사용하여 ../common/ 같은 경로를 제거하고 파일명만 남긴 후 .obj/를 붙입니다.
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))

# 3. VPATH 설정: make가 실제 소스 위치를 찾을 디렉토리 목록 지정 
# make가 .obj/ 아래에 없는 소스 파일을 찾을 때 참조할 경로입니다.
VPATH = $(SRC_DIR) : $(COMMON_DIR)

# 4. 생성된 모든 .d 파일 목록 정의
DEPS = $(OBJS:.o=.d)

# 5. 인클루드 경로 설정
INC_DIRS = ../common/inc inc
INC_DIRS += $(shell find . -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

FINAL_TARGET = $(if $(OUTPUT_DIR), $(OUTPUT_DIR)/$(TARGET), ../$(TARGET))

all: $(FINAL_TARGET)

$(FINAL_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@
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
