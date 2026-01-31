# ---------------------------------------------------------
# 최상위 Makefile
# ---------------------------------------------------------
CC ?= gcc
export CC  # 하위 Makefile들로 컴파일러 정보를 전달

ifneq ($(findstring arm,$(CC)),)
CFLAGS += -march=armv7-a -mfpu=neon  # ARM 전용 최적화 옵션 예시
endif

# 빌드할 하위 모듈 목록
SUBDIRS = boss parent child1 child2 child3
# 실행 파일이 모일 출력 폴더
export OUTPUT_DIR = $(CURDIR)/output

# 개별 clean 타겟 목록 생성 (boss-clean, parent-clean 등)
CLEAN_TARGETS = $(addsuffix -clean, $(SUBDIRS))

all: create_output
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done
	@echo "\n[Success] All binaries are located in: $(OUTPUT_DIR)"

# 각 프로세스별 개별 빌드 규칙
# 이 설정 덕분에 'make boss' 또는 'make parent' 명령이 가능해집니다.
$(SUBDIRS):
	@echo "\n>>> Entering directory: [ $@ ]"
	@$(MAKE) -C $@
	@echo "<<< Leaving directory: [ $@ ]"

create_output:
	@mkdir -p $(OUTPUT_DIR)

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean || exit 1; \
	done
	rm -rf $(OUTPUT_DIR)
	@echo "Cleaned all build artifacts."

$(CLEAN_TARGETS):
	@dir=$(subst -clean,,$@); \
	echo "\n>>> Cleaning directory: [ $$dir ]"; \
	$(MAKE) -C $$dir clean


.PHONY: all clean create_output $(SUBDIRS)
