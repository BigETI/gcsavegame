#pragma once

#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#define GC_BANNER_WIDTH			(96)
#define GC_BANNER_HEIGHT		(32)
#define GC_BANNER_PIXEL_COUNT	(GC_BANNER_WIDTH * GC_BANNER_HEIGHT)
#define GC_ICON_WIDTH			(32)
#define GC_ICON_HEIGHT			(32)
#define GC_ICON_PIXEL_COUNT		(GC_ICON_WIDTH * GC_ICON_HEIGHT)

class GCMem
{
public:
	class DirEntry
	{
	private:
		char game_code[0x4], maker_code[0x2];
		uint8_t dummy1, graphics_format;
		char game_title[0x20];
		uint32_t save_time, image_offset;
		uint16_t icon_format, animation_speed;
		uint8_t file_permissions, counter;
		uint16_t block_index, save_size, dummy2;
		uint32_t comment_offset;
	public:
		enum REGION
		{
			REGION_JAPAN,
			REGION_USA,
			REGION_EUR,
			REGION_UNKNOWN
		};
		enum COLOR_FORMAT
		{
			COLOR_FORMAT_RGB5A3,
			COLOR_FORMAT_CI8
		};
		enum ICON_ANIMATION_TYPE
		{
			ICON_ANIMATION_FORWARD,
			ICON_ANIMATION_PING_PONG
		};
		enum ICON_FORMAT
		{
			ICON_FORMAT_NONE,
			ICON_FORMAT_CI8_REUSE_PALETTE,
			ICON_FORMAT_RGB5A3,
			ICON_FORMAT_CI8_UNIQUE_PALETTE
		};
		enum ANIMATION_SPEED
		{
			ANIMATION_SPEED_NONE,
			ANIMATION_SPEED_4,
			ANIMATION_SPEED_8,
			ANIMATION_SPEED_12
		};
		DirEntry(FILE *file);
		~DirEntry();
		void __fastcall Read(FILE *file);
		bool __fastcall IsValid();
		REGION __fastcall GetRegion();
		std::string __fastcall GetRegionName();
		std::string __fastcall GetGameCode();
		std::string __fastcall GetMakerCode();
		std::string __fastcall GetMakerName();
		COLOR_FORMAT __fastcall GetColorFormat();
		std::string __fastcall GetColorFormatName();
		bool __fastcall IsBannerPresent();
		ICON_ANIMATION_TYPE __fastcall GetIconAnimationType();
		std::string __fastcall GetIconAnimationTypeName();
		std::string __fastcall GetGameTitle();
		uint32_t __fastcall GetSaveTimeStamp();
		std::string __fastcall GetSaveTime();
		uint32_t __fastcall GetBannerOffsetInSave();
		uint32_t __fastcall GetBannerOffsetInFile();
		ICON_FORMAT __fastcall GetIconFormat(uint8_t index);
		std::string __fastcall GetIconFormatName(uint8_t index);
		ANIMATION_SPEED __fastcall GetAnimationSpeed(uint8_t index);
		std::string __fastcall GetAnimationSpeedName(uint8_t index);
		bool __fastcall IsSavePublic();
		bool __fastcall CopyDisabled();
		bool __fastcall MovingDisabled();
		uint8_t __fastcall GetCounter();
		uint16_t __fastcall GetBlockIndex();
		uint32_t __fastcall GetSaveDataOffsetInFile();
		uint16_t __fastcall GetSaveSize();
		uint32_t __fastcall GetComment1OffsetInSave();
		uint32_t __fastcall GetComment1OffsetInFile();
		uint32_t __fastcall GetComment2OffsetInSave();
		uint32_t __fastcall GetComment2OffsetInFile();
	};

	class GameData
	{
	private:
		DirEntry *dir_entry;
		struct Pixel
		{
			uint8_t blue, green, red, alpha;
		} banner_pixel_data[GC_BANNER_PIXEL_COUNT], icon_pixel_data[8][GC_ICON_PIXEL_COUNT];
		std::string comment1, comment2;

	public:
		GameData(FILE *file);
		~GameData();
		bool __fastcall IsValid();
		DirEntry &__fastcall GetDirEntry();
		std::string __fastcall GetComment1();
		std::string __fastcall GetComment2();
		void __fastcall StoreBannerAsTGA(std::string file_name);
		void __fastcall StoreIconAsTGA(std::string file_name, uint8_t index);
		void __fastcall StoreIconsAsTGA(std::string file_name);
	};
	GCMem(std::string file_name);
	~GCMem();
	GameData &__fastcall Game(uint8_t index);
	uint8_t __fastcall GameCount();
private:
	std::vector<GameData *> games;
};