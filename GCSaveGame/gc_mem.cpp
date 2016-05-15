#include "gc_mem.h"
#include <ctime>

#define Convert5To8(_v)			(((((int16_t)(_v)) << 3) | (((int16_t)(_v)) >> 2)) & 0xFF)
#define Convert3To8(_v)			(((((int16_t)(_v)) << 5) | (((int16_t)(_v)) << 2) | (((int16_t)(_v)) >> 1)) & 0xFF)
#define Convert4To8(_v)			(((((int16_t)(_v)) << 4) | ((int16_t)(_v))) & 0xFF)

#define GC_PALETTE_COUNT	(0x100)

#define TGA_CMP_HEADER_LEN		(12)

struct IHDR_chunk
{
	unsigned short width, height;
	unsigned char bpp, dummy;
};

static const IHDR_chunk banner_ihdr_chunk_buffer = { GC_BANNER_WIDTH, GC_BANNER_HEIGHT, 32, 0 }, s_icon_ihdr_chunk_buffer = { GC_ICON_WIDTH, GC_ICON_HEIGHT, 32, 0 }, m_icon_ihdr_chunk_buffer = { GC_ICON_WIDTH, GC_ICON_HEIGHT * 8, 32, 0 };
static const char tga_header[TGA_CMP_HEADER_LEN] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

GCMem::DirEntry::DirEntry(FILE *file)
{
	Read(file);
}

GCMem::DirEntry::~DirEntry()
{

}

void __fastcall GCMem::DirEntry::Read(FILE *file)
{
	if (fread(this, sizeof(DirEntry), 1, file) != 1) memset(this, 0xFF, sizeof(DirEntry));
}

bool __fastcall GCMem::DirEntry::IsValid()
{
	return (*((uint32_t *)game_code) != 0xFFFFFFFF);
}

GCMem::DirEntry::REGION __fastcall GCMem::DirEntry::GetRegion()
{
	REGION ret = REGION_UNKNOWN;
	switch (game_code[3])
	{
	case 'J': ret = REGION_JAPAN; break;
	case 'E': ret = REGION_USA; break;
	case 'P': ret = REGION_EUR; break;
	}
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetRegionName()
{
	std::string ret = "Unknown";
	switch (game_code[3])
	{
	case 'J': ret = "Japan"; break;
	case 'E': ret = "USA"; break;
	case 'P': ret = "Europe"; break;
	}
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetGameCode()
{
	std::string ret;
	for (int i = 0; i < 0x4; i++) ret.push_back(game_code[i]);
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetMakerCode()
{
	std::string ret;
	for (int i = 0; i < 0x2; i++) ret.push_back(maker_code[i]);
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetMakerName()
{
	std::string ret = "Unknown";
	switch (*((uint16_t *)maker_code))
	{
	case 0x3130: ret = "Nintendo"; break;
	case 0x3830: ret = "Capcom"; break;
	case 0x3134: ret = "Ubisoft"; break;
	case 0x4634: ret = "Eidos"; break;
	case 0x3135: ret = "Acclaim"; break;
	case 0x3235: ret = "Activision"; break;
	case 0x4435: ret = "Midway"; break;
	case 0x4735: ret = "Hudson"; break;
	case 0x3436: ret = "Lucas Arts"; break;
	case 0x3936: ret = "Electronic Arts"; break;
	case 0x5336: ret = "TDK Mediactive"; break;
	case 0x5038: ret = "Sega"; break;
	case 0x3441: ret = "Mirage Studios"; break;
	case 0x4641: ret = "Namco"; break;
	case 0x3242: ret = "Bandai"; break;
	case 0x4144: ret = "Tomy"; break;
	case 0x4D45: ret = "Konami"; break;
	}
	return ret;
}

GCMem::DirEntry::COLOR_FORMAT __fastcall GCMem::DirEntry::GetColorFormat()
{
	return (graphics_format & 0x1) ? COLOR_FORMAT_CI8 : COLOR_FORMAT_RGB5A3;
}

std::string __fastcall GCMem::DirEntry::GetColorFormatName()
{
	std::string ret;
	if (graphics_format & 0x1) ret = "CI8";
	else ret = "RGB5A3";
	return ret;
}

bool __fastcall GCMem::DirEntry::IsBannerPresent()
{
	return ((bool)(graphics_format & 0x1)) != ((bool)(graphics_format & 0x2));
}

GCMem::DirEntry::ICON_ANIMATION_TYPE __fastcall GCMem::DirEntry::GetIconAnimationType()
{
	return (graphics_format & 0x3) ? ICON_ANIMATION_FORWARD : ICON_ANIMATION_PING_PONG;
}

std::string __fastcall GCMem::DirEntry::GetIconAnimationTypeName()
{
	return (graphics_format & 0x3) ? "forward" : "ping pong";
}

std::string __fastcall GCMem::DirEntry::GetGameTitle()
{
	std::string ret;
	for (int i = 0; i < 0x20; i++)
	{
		if (game_title[i] == '\0') break;
		ret.push_back(game_title[i]);
	}
	return ret;
}

uint32_t __fastcall GCMem::DirEntry::GetSaveTimeStamp()
{
	return (((save_time >> 24) & 0xFF) | ((save_time >> 8) & 0xFF00) | ((save_time << 8) & 0xFF0000) | ((save_time << 24) & 0xFF000000)) + 946684800;
}

std::string __fastcall GCMem::DirEntry::GetSaveTime()
{
	time_t time(GetSaveTimeStamp());
	struct tm _tm;
	localtime_s(&_tm, &time);
	char buffer[32];
	std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", &_tm);
	return buffer;
}

uint32_t __fastcall GCMem::DirEntry::GetBannerOffsetInSave()
{
	return ((image_offset >> 24) & 0xFF) | ((image_offset >> 8) & 0xFF00) | ((image_offset << 8) & 0xFF0000) | ((image_offset << 24) & 0xFF000000);
}

uint32_t __fastcall GCMem::DirEntry::GetBannerOffsetInFile()
{
	return GetSaveDataOffsetInFile() + GetBannerOffsetInSave();
}

GCMem::DirEntry::ICON_FORMAT __fastcall GCMem::DirEntry::GetIconFormat(uint8_t index)
{
	ICON_FORMAT ret = ICON_FORMAT_NONE;
	if (index < 8)
	{
		//index = 7 - index;
		switch (((((icon_format >> 8) & 0xFF) | ((icon_format << 8) & 0xFF00)) >> (index * 2)) & 0x3)
		{
		case 0x1: ret = ICON_FORMAT_CI8_REUSE_PALETTE; break;
		case 0x2: ret = ICON_FORMAT_RGB5A3; break;
		case 0x3: ret = ICON_FORMAT_CI8_UNIQUE_PALETTE; break;
		}
	}
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetIconFormatName(uint8_t index)
{
	std::string ret = "Unknown";
	if (index < 8)
	{
		//index = 7 - index;
		switch (((((icon_format >> 8) & 0xFF) | ((icon_format << 8) & 0xFF00)) >> (index * 2)) & 0x3)
		{
		case 0x1: ret = "CI8 (reuse palette)"; break;
		case 0x2: ret = "RGB5A3"; break;
		case 0x3: ret = "CI8 (unique palette)"; break;
		}
	}
	return ret;
}

GCMem::DirEntry::ANIMATION_SPEED __fastcall GCMem::DirEntry::GetAnimationSpeed(uint8_t index)
{
	ANIMATION_SPEED ret = ANIMATION_SPEED_NONE;
	if (index < 8)
	{
		//index = 7 - index;
		switch (((((animation_speed >> 8) & 0xFF) | ((animation_speed << 8) & 0xFF00)) >> (index * 2)) & 0x3)
		{
		case 0x1: ret = ANIMATION_SPEED_4; break;
		case 0x2: ret = ANIMATION_SPEED_8; break;
		case 0x3: ret = ANIMATION_SPEED_12; break;
		}
	}
	return ret;
}

std::string __fastcall GCMem::DirEntry::GetAnimationSpeedName(uint8_t index)
{
	std::string ret = "None";
	if (index < 8)
	{
		//index = 7 - index;
		switch (((((animation_speed >> 8) & 0xFF) | ((animation_speed << 8) & 0xFF00)) >> (index * 2)) & 0x3)
		{
		case 0x1: ret = "4"; break;
		case 0x2: ret = "8"; break;
		case 0x3: ret = "12"; break;
		}
	}
	return ret;
}

bool __fastcall GCMem::DirEntry::IsSavePublic()
{
	return (file_permissions & 0x4) != 0;
}

bool __fastcall GCMem::DirEntry::CopyDisabled()
{
	return (file_permissions & 0x8) != 0;
}

bool __fastcall GCMem::DirEntry::MovingDisabled()
{
	return (file_permissions & 0x10) != 0;
}

uint8_t __fastcall GCMem::DirEntry::GetCounter()
{
	return counter;
}

uint16_t __fastcall GCMem::DirEntry::GetBlockIndex()
{
	return ((block_index >> 8) & 0xFF) | ((block_index << 8) & 0xFF00);
}

uint32_t __fastcall GCMem::DirEntry::GetSaveDataOffsetInFile()
{
	return (GetBlockIndex() * 0x2000);
}

uint16_t __fastcall GCMem::DirEntry::GetSaveSize()
{
	return ((save_size >> 8) & 0xFF) | ((save_size << 8) & 0xFF00);
}

uint32_t __fastcall GCMem::DirEntry::GetComment1OffsetInSave()
{
	return ((comment_offset >> 24) & 0xFF) | ((comment_offset >> 8) & 0xFF00) | ((comment_offset << 8) & 0xFF0000) | ((comment_offset << 24) & 0xFF000000);
}

uint32_t __fastcall GCMem::DirEntry::GetComment1OffsetInFile()
{
	return GetSaveDataOffsetInFile() + GetComment1OffsetInSave();
}

uint32_t __fastcall GCMem::DirEntry::GetComment2OffsetInSave()
{
	return GetComment1OffsetInSave() + 0x20;
}

uint32_t __fastcall GCMem::DirEntry::GetComment2OffsetInFile()
{
	return GetSaveDataOffsetInFile() + GetComment2OffsetInSave();
}

GCMem::GameData::GameData(FILE *file) : dir_entry(NULL)
{
	const int tile_height = GC_BANNER_HEIGHT / 4, rgb53a_tile_width = GC_BANNER_WIDTH / 4, ci8_tile_width = GC_BANNER_WIDTH / 8, icon_tile_height = GC_ICON_HEIGHT / 4, rgb53a_icon_tile_width = GC_ICON_WIDTH / 4, ci8_icon_tile_width = GC_ICON_WIDTH / 8;
	Pixel t_banner_pixel_data[GC_BANNER_PIXEL_COUNT], t_icon_pixel_data[GC_ICON_PIXEL_COUNT];
	long current_pos, icon_pos;
	int i, j, k, l, m, raw_i, out_i, palette_index;
	char c;
	void *input_pixel = NULL;
	uint16_t palette[GC_PALETTE_COUNT];
	memset(banner_pixel_data, 0, sizeof(Pixel) * GC_BANNER_PIXEL_COUNT);
	memset(t_banner_pixel_data, 0, sizeof(Pixel) * GC_BANNER_PIXEL_COUNT);
	memset(icon_pixel_data, 0, sizeof(Pixel) * 8 * GC_ICON_PIXEL_COUNT);
	memset(t_icon_pixel_data, 0, sizeof(Pixel) * GC_ICON_PIXEL_COUNT);
	memset(palette, 0, sizeof(uint16_t) * GC_PALETTE_COUNT);
	//memset(u_palette, 0, sizeof(uint16_t) * 8 * GC_PALETTE_COUNT);
	try
	{
		dir_entry = new DirEntry(file);
		current_pos = ftell(file);
	}
	catch (...)
	{
		dir_entry = NULL;
	}
	if (dir_entry)
	{
		if (dir_entry->IsValid())
		{
			fseek(file, dir_entry->GetComment1OffsetInFile(), SEEK_SET);
			for (i = 0; i < 0x20; i++)
			{
				if ((c = fgetc(file)) == '\0') break;
				comment1.push_back(c);
			}
			fseek(file, dir_entry->GetComment2OffsetInFile(), SEEK_SET);
			for (i = 0; i < 0x20; i++)
			{
				if ((c = fgetc(file)) == '\0') break;
				comment2.push_back(c);
			}
			fseek(file, dir_entry->GetBannerOffsetInFile(), SEEK_SET);
			switch (dir_entry->GetColorFormat())
			{
			case DirEntry::COLOR_FORMAT_RGB5A3:
				try
				{
					input_pixel = ((void *)(new uint16_t[GC_BANNER_PIXEL_COUNT]));
				}
				catch (...)
				{
					input_pixel = NULL;
				}
				if (input_pixel)
				{
					fread(input_pixel, sizeof(uint16_t), GC_BANNER_PIXEL_COUNT, file);
					for (i = 0; i < GC_BANNER_PIXEL_COUNT; i++) (((uint8_t *)(&(((uint16_t *)input_pixel)[i])))[0] ^= (((uint8_t *)(&(((uint16_t *)input_pixel)[i])))[1] ^= (((uint8_t *)(&(((uint16_t *)input_pixel)[i])))[0] ^= ((uint8_t *)(&(((uint16_t *)input_pixel)[i])))[1])));
					for (i = 0; i < tile_height; i++)
					{
						for (j = 0; j < rgb53a_tile_width; j++)
						{
							for (k = 0; k < 4; k++)
							{
								for (l = 0; l < 4; l++)
								{
									out_i = (j * 4) + (i * GC_BANNER_WIDTH * 4) + (GC_BANNER_WIDTH * k) + l;
									if (((uint16_t *)input_pixel)[raw_i = (j * 16) + (i * GC_BANNER_WIDTH * 4) + (k * 4) + l] & 0x8000)
									{
										t_banner_pixel_data[out_i].red = Convert5To8(((((uint16_t *)input_pixel)[raw_i] >> 10) & 0x1F));
										t_banner_pixel_data[out_i].green = Convert5To8(((((uint16_t *)input_pixel)[raw_i] >> 5) & 0x1F));
										t_banner_pixel_data[out_i].blue = Convert5To8((((uint16_t *)input_pixel)[raw_i] & 0x1F));
										t_banner_pixel_data[out_i].alpha = 0xFF;
									}
									else
									{
										t_banner_pixel_data[out_i].alpha = Convert3To8((((uint16_t *)input_pixel)[raw_i] >> 12) & 0x7);
										t_banner_pixel_data[out_i].red = Convert4To8((((uint16_t *)input_pixel)[raw_i] >> 8) & 0xF);
										t_banner_pixel_data[out_i].green = Convert4To8((((uint16_t *)input_pixel)[raw_i] >> 4) & 0xF);
										t_banner_pixel_data[out_i].blue = Convert4To8(((uint16_t *)input_pixel)[raw_i] & 0xF);
									}
								}
							}
						}
					}
					for (i = 0; i < GC_BANNER_HEIGHT; i++) memcpy(&(banner_pixel_data[i * GC_BANNER_WIDTH]), &(t_banner_pixel_data[GC_BANNER_PIXEL_COUNT - ((i + 1) * GC_BANNER_WIDTH)]), sizeof(Pixel) * GC_BANNER_WIDTH);
					delete[]((uint16_t *)input_pixel);
				}
				break;
			case DirEntry::COLOR_FORMAT_CI8:
				try
				{
					input_pixel = ((void *)(new uint8_t[GC_BANNER_PIXEL_COUNT]));
				}
				catch (...)
				{
					input_pixel = NULL;
				}
				if (input_pixel)
				{
					fread(input_pixel, sizeof(uint8_t), GC_BANNER_PIXEL_COUNT, file);
					fread(palette, sizeof(uint16_t), GC_PALETTE_COUNT, file);
					for (i = 0; i < GC_PALETTE_COUNT; i++) (((uint8_t *)(&(((uint16_t *)palette)[i])))[0] ^= (((uint8_t *)(&(((uint16_t *)palette)[i])))[1] ^= (((uint8_t *)(&(((uint16_t *)palette)[i])))[0] ^= ((uint8_t *)(&(((uint16_t *)palette)[i])))[1])));
					for (i = 0; i < tile_height; i++)
					{
						for (j = 0; j < ci8_tile_width; j++)
						{
							for (k = 0; k < 4; k++)
							{
								for (l = 0; l < 8; l++)
								{
									out_i = (j * 8) + (i * GC_BANNER_WIDTH * 4) + (GC_BANNER_WIDTH * k) + l;
									palette_index = (int)(((uint8_t *)input_pixel)[(j * 32) + (i * GC_BANNER_WIDTH * 4) + (k * 8) + l]);
									if (palette[palette_index] & 0x8000)
									{
										t_banner_pixel_data[out_i].red = Convert5To8(((palette[palette_index] >> 10) & 0x1F));
										t_banner_pixel_data[out_i].green = Convert5To8(((palette[palette_index] >> 5) & 0x1F));
										t_banner_pixel_data[out_i].blue = Convert5To8((palette[palette_index] & 0x1F));
										t_banner_pixel_data[out_i].alpha = 0xFF;
									}
									else
									{
										t_banner_pixel_data[out_i].alpha = Convert3To8((palette[palette_index] >> 12) & 0x7);
										t_banner_pixel_data[out_i].red = Convert4To8((palette[palette_index] >> 8) & 0xF);
										t_banner_pixel_data[out_i].green = Convert4To8((palette[palette_index] >> 4) & 0xF);
										t_banner_pixel_data[out_i].blue = Convert4To8(palette[palette_index] & 0xF);
									}
								}
							}
						}
					}
					for (i = 0; i < GC_BANNER_HEIGHT; i++) memcpy(&(banner_pixel_data[i * GC_BANNER_WIDTH]), &(t_banner_pixel_data[GC_BANNER_PIXEL_COUNT - ((i + 1) * GC_BANNER_WIDTH)]), sizeof(Pixel) * GC_BANNER_WIDTH);
					delete[]((uint8_t *)input_pixel);
				}
				break;
			}
			icon_pos = ftell(file);
			for (i = 0; i < 8; i++)
			{
				switch (dir_entry->GetIconFormat(i))
				{
				case DirEntry::ICON_FORMAT_CI8_REUSE_PALETTE:
					fseek(file, GC_ICON_PIXEL_COUNT * sizeof(uint8_t), SEEK_CUR);
					break;
				case DirEntry::ICON_FORMAT_RGB5A3:
					try
					{
						input_pixel = new uint16_t[GC_ICON_PIXEL_COUNT];
					}
					catch (...)
					{
						input_pixel = NULL;
					}
					if (input_pixel)
					{
						fread(input_pixel, sizeof(uint16_t), GC_ICON_PIXEL_COUNT, file);
						for (j = 0; j < GC_ICON_PIXEL_COUNT; j++) (((uint8_t *)(&(((uint16_t *)input_pixel)[j])))[0] ^= (((uint8_t *)(&(((uint16_t *)input_pixel)[j])))[1] ^= (((uint8_t *)(&(((uint16_t *)input_pixel)[j])))[0] ^= ((uint8_t *)(&(((uint16_t *)input_pixel)[j])))[1])));
						for (j = 0; j < icon_tile_height; j++)
						{
							for (k = 0; k < rgb53a_icon_tile_width; k++)
							{
								for (l = 0; l < 4; l++)
								{
									for (m = 0; m < 4; m++)
									{
										out_i = (k * 4) + (j * GC_ICON_WIDTH * 4) + (GC_ICON_WIDTH * l) + m;
										if (((uint16_t *)input_pixel)[raw_i = (k * 16) + (j * GC_ICON_WIDTH * 4) + (l * 4) + m] & 0x8000)
										{
											t_icon_pixel_data[out_i].red = Convert5To8(((((uint16_t *)input_pixel)[raw_i] >> 10) & 0x1F));
											t_icon_pixel_data[out_i].green = Convert5To8(((((uint16_t *)input_pixel)[raw_i] >> 5) & 0x1F));
											t_icon_pixel_data[out_i].blue = Convert5To8((((uint16_t *)input_pixel)[raw_i] & 0x1F));
											t_icon_pixel_data[out_i].alpha = 0xFF;
										}
										else
										{
											t_icon_pixel_data[out_i].alpha = Convert3To8((((uint16_t *)input_pixel)[raw_i] >> 12) & 0x7);
											t_icon_pixel_data[out_i].red = Convert4To8((((uint16_t *)input_pixel)[raw_i] >> 8) & 0xF);
											t_icon_pixel_data[out_i].green = Convert4To8((((uint16_t *)input_pixel)[raw_i] >> 4) & 0xF);
											t_icon_pixel_data[out_i].blue = Convert4To8(((uint16_t *)input_pixel)[raw_i] & 0xF);
										}
									}
								}
							}
						}
						for (j = 0; j < GC_ICON_HEIGHT; j++) memcpy(&(icon_pixel_data[i][j * GC_ICON_WIDTH]), &(t_icon_pixel_data[GC_ICON_PIXEL_COUNT - ((j + 1) * GC_ICON_WIDTH)]), sizeof(Pixel) * GC_ICON_WIDTH);
						delete[]((uint16_t *)input_pixel);
					}
					else fseek(file, GC_ICON_PIXEL_COUNT * sizeof(uint16_t), SEEK_CUR);
					break;
				case DirEntry::ICON_FORMAT_CI8_UNIQUE_PALETTE:
					try
					{
						input_pixel = new uint8_t[GC_ICON_PIXEL_COUNT];
					}
					catch (...)
					{
						input_pixel = NULL;
					}
					if (input_pixel)
					{
						fread(input_pixel, sizeof(uint8_t), GC_ICON_PIXEL_COUNT, file);
						fread(palette, sizeof(uint16_t), GC_PALETTE_COUNT, file);
						for (j = 0; j < GC_PALETTE_COUNT; j++) (((uint8_t *)(&(((uint16_t *)palette)[j])))[0] ^= (((uint8_t *)(&(((uint16_t *)palette)[j])))[1] ^= (((uint8_t *)(&(((uint16_t *)palette)[j])))[0] ^= ((uint8_t *)(&(((uint16_t *)palette)[j])))[1])));
						for (j = 0; j < icon_tile_height; j++)
						{
							for (k = 0; k < ci8_icon_tile_width; k++)
							{
								for (l = 0; l < 4; l++)
								{
									for (m = 0; m < 8; m++)
									{
										out_i = (k * 8) + (j * GC_ICON_WIDTH * 4) + (GC_ICON_WIDTH * l) + m;
										palette_index = (int)(((uint8_t *)input_pixel)[(k * 32) + (j * GC_ICON_WIDTH * 4) + (l * 8) + m]);
										if (palette[palette_index] & 0x8000)
										{
											t_icon_pixel_data[out_i].red = Convert5To8(((palette[palette_index] >> 10) & 0x1F));
											t_icon_pixel_data[out_i].green = Convert5To8(((palette[palette_index] >> 5) & 0x1F));
											t_icon_pixel_data[out_i].blue = Convert5To8((palette[palette_index] & 0x1F));
											t_icon_pixel_data[out_i].alpha = 0xFF;
										}
										else
										{
											t_icon_pixel_data[out_i].alpha = Convert3To8((palette[palette_index] >> 12) & 0x7);
											t_icon_pixel_data[out_i].red = Convert4To8((palette[palette_index] >> 8) & 0xF);
											t_icon_pixel_data[out_i].green = Convert4To8((palette[palette_index] >> 4) & 0xF);
											t_icon_pixel_data[out_i].blue = Convert4To8(palette[palette_index] & 0xF);
										}
									}
								}
							}
						}
						for (j = 0; j < GC_ICON_HEIGHT; j++) memcpy(&(icon_pixel_data[i][j * GC_ICON_WIDTH]), &(t_icon_pixel_data[GC_ICON_PIXEL_COUNT - ((j + 1) * GC_ICON_WIDTH)]), sizeof(Pixel) * GC_ICON_WIDTH);
						delete[]((uint8_t *)input_pixel);
					}
					else fseek(file, (sizeof(uint8_t) * GC_ICON_PIXEL_COUNT) + (sizeof(uint16_t) * GC_PALETTE_COUNT), SEEK_CUR);
					break;
				}
			}
			fread(palette, sizeof(uint16_t), GC_PALETTE_COUNT, file);
			for (i = 0; i < GC_PALETTE_COUNT; i++) (((uint8_t *)(&(((uint16_t *)palette)[i])))[0] ^= (((uint8_t *)(&(((uint16_t *)palette)[i])))[1] ^= (((uint8_t *)(&(((uint16_t *)palette)[i])))[0] ^= ((uint8_t *)(&(((uint16_t *)palette)[i])))[1])));
			fseek(file, icon_pos, SEEK_SET);
			for (i = 0; i < 8; i++)
			{
				switch (dir_entry->GetIconFormat(i))
				{
				case DirEntry::ICON_FORMAT_CI8_REUSE_PALETTE:
					try
					{
						input_pixel = new uint8_t[GC_ICON_PIXEL_COUNT];
					}
					catch (...)
					{
						input_pixel = NULL;
					}
					if (input_pixel)
					{
						fread(input_pixel, sizeof(uint8_t), GC_ICON_PIXEL_COUNT, file);
						for (j = 0; j < icon_tile_height; j++)
						{
							for (k = 0; k < ci8_icon_tile_width; k++)
							{
								for (l = 0; l < 4; l++)
								{
									for (m = 0; m < 8; m++)
									{
										out_i = (k * 8) + (j * GC_ICON_WIDTH * 4) + (GC_ICON_WIDTH * l) + m;
										palette_index = (int)(((uint8_t *)input_pixel)[(k * 32) + (j * GC_ICON_WIDTH * 4) + (l * 8) + m]);
										if (palette[palette_index] & 0x8000)
										{
											t_icon_pixel_data[out_i].red = Convert5To8(((palette[palette_index] >> 10) & 0x1F));
											t_icon_pixel_data[out_i].green = Convert5To8(((palette[palette_index] >> 5) & 0x1F));
											t_icon_pixel_data[out_i].blue = Convert5To8((palette[palette_index] & 0x1F));
											t_icon_pixel_data[out_i].alpha = 0xFF;
										}
										else
										{
											t_icon_pixel_data[out_i].alpha = Convert3To8((palette[palette_index] >> 12) & 0x7);
											t_icon_pixel_data[out_i].red = Convert4To8((palette[palette_index] >> 8) & 0xF);
											t_icon_pixel_data[out_i].green = Convert4To8((palette[palette_index] >> 4) & 0xF);
											t_icon_pixel_data[out_i].blue = Convert4To8(palette[palette_index] & 0xF);
										}
									}
								}
							}
						}
						for (j = 0; j < GC_ICON_HEIGHT; j++) memcpy(&(icon_pixel_data[i][j * GC_ICON_WIDTH]), &(t_icon_pixel_data[GC_ICON_PIXEL_COUNT - ((j + 1) * GC_ICON_WIDTH)]), sizeof(Pixel) * GC_ICON_WIDTH);
						delete[]((uint8_t *)input_pixel);
					}
					else fseek(file, sizeof(uint8_t) * GC_ICON_PIXEL_COUNT, SEEK_CUR);
					break;
				case DirEntry::ICON_FORMAT_RGB5A3:
					fseek(file, GC_ICON_PIXEL_COUNT * sizeof(uint16_t), SEEK_CUR);
					break;
				case DirEntry::ICON_FORMAT_CI8_UNIQUE_PALETTE:
					fseek(file, (sizeof(uint8_t) * GC_ICON_PIXEL_COUNT) + (sizeof(uint16_t) * GC_PALETTE_COUNT), SEEK_CUR);
					break;
				}
			}
			fseek(file, current_pos, SEEK_SET);
		}
	}
}

GCMem::GameData::~GameData()
{
	delete dir_entry;
	dir_entry = NULL;
}

bool __fastcall GCMem::GameData::IsValid()
{
	return dir_entry ? dir_entry->IsValid() : false;
}

GCMem::DirEntry &__fastcall GCMem::GameData::GetDirEntry()
{
	return *dir_entry;
}

std::string __fastcall GCMem::GameData::GetComment1()
{
	return comment1;
}

std::string __fastcall GCMem::GameData::GetComment2()
{
	return comment2;
}

void __fastcall GCMem::GameData::StoreBannerAsTGA(std::string file_name)
{
	FILE *file = NULL;
	fopen_s(&file, file_name.c_str(), "wb");
	if (file)
	{
		fwrite(tga_header, sizeof(char), TGA_CMP_HEADER_LEN, file);
		fwrite(&banner_ihdr_chunk_buffer, sizeof(IHDR_chunk), 1, file);
		fwrite(banner_pixel_data, sizeof(Pixel), GC_BANNER_PIXEL_COUNT, file);
		fclose(file);
	}
}

void __fastcall GCMem::GameData::StoreIconAsTGA(std::string file_name, uint8_t index)
{
	FILE *file = NULL;
	if (index < 8)
	{
		fopen_s(&file, file_name.c_str(), "wb");
		if (file)
		{
			fwrite(tga_header, sizeof(char), TGA_CMP_HEADER_LEN, file);
			fwrite(&s_icon_ihdr_chunk_buffer, sizeof(IHDR_chunk), 1, file);
			fwrite(icon_pixel_data[index], sizeof(Pixel), GC_ICON_PIXEL_COUNT, file);
			fclose(file);
		}
	}
}

void __fastcall GCMem::GameData::StoreIconsAsTGA(std::string file_name)
{
	FILE *file = NULL;
	fopen_s(&file, file_name.c_str(), "wb");
	if (file)
	{
		fwrite(tga_header, sizeof(char), TGA_CMP_HEADER_LEN, file);
		fwrite(&m_icon_ihdr_chunk_buffer, sizeof(IHDR_chunk), 1, file);
		fwrite(*icon_pixel_data, sizeof(Pixel), GC_ICON_PIXEL_COUNT * 8, file);
		fclose(file);
	}
}

GCMem::GCMem(std::string file_name)
{
	FILE *file = NULL;
	long long file_size;
	int i;
	GameData *game_data = NULL;
	std::vector<GameData *>::iterator game_it;
	fopen_s(&file, file_name.c_str(), "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		file_size = _ftelli64(file);
		fseek(file, 0, SEEK_SET);
		if (file_size == 16777216)
		{
			fseek(file, 0x2000, SEEK_SET);
			for (i = 0; i < 0x80; i++)
			{
				if (game_data) game_data->GetDirEntry().Read(file);
				else
				{
					try
					{
						game_data = new GCMem::GameData(file);
					}
					catch (...)
					{
						game_data = NULL;
					}
					if (game_data == NULL) break;
				}
				if (game_data->IsValid())
				{
					try
					{
						games.push_back(game_data);
						game_data = NULL;
					}
					catch (...)
					{
						delete game_data;
						game_data = NULL;
						for (game_it = games.begin(); game_it != games.end(); game_it++) delete (*game_it);
						games.clear();
					}
				}
			}
		}
		fclose(file);
	}
}

GCMem::~GCMem()
{
	std::vector<GameData *>::iterator game_it;
	for (game_it = games.begin(); game_it != games.end(); game_it++) delete (*game_it);
	games.clear();
}

GCMem::GameData &__fastcall GCMem::Game(uint8_t index)
{
	if (index >= games.size()) throw -1;
	return (*(games[index]));
}

uint8_t __fastcall GCMem::GameCount()
{
	return ((uint8_t)(games.size()));
}