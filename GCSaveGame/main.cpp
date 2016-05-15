#include "gc_mem.h"

int main(unsigned int argc, char *argv[])
{
	GCMem *gc_mem = NULL;
	int i;
	unsigned int game_index = 0, icon_index;
	if (argc > 1 /*true*/)
	{
		try
		{
			gc_mem = new GCMem(argv[1] /*"MemoryCardA.USA.raw"*/);
		}
		catch (...)
		{
			gc_mem = NULL;
		}
		if (gc_mem)
		{
			for (i = 0; i < gc_mem->GameCount(); i++)
			{
				std::cout << "\n\t" << (++game_index)
					<< ". " << gc_mem->Game(i).GetDirEntry().GetGameTitle()
					<< ":\n\t\tCode: " << gc_mem->Game(i).GetDirEntry().GetGameCode()
					<< " (" << gc_mem->Game(i).GetDirEntry().GetRegionName()
					<< ")\n\t\tMaker: " << gc_mem->Game(i).GetDirEntry().GetMakerName() << " \"" << gc_mem->Game(i).GetDirEntry().GetMakerCode()
					<< "\"\n\t\tColor format: " << gc_mem->Game(i).GetDirEntry().GetColorFormatName()
					<< "\n\t\tIs banner present: " << (gc_mem->Game(i).GetDirEntry().IsBannerPresent() ? "yes" : "no")
					<< "\n\t\tAnimation type: " << gc_mem->Game(i).GetDirEntry().GetIconAnimationTypeName()
					<< "\n\t\tSave time: " << gc_mem->Game(i).GetDirEntry().GetSaveTime()
					<< "\n\t\tImage offset: " << gc_mem->Game(i).GetDirEntry().GetBannerOffsetInFile() << " ( " << gc_mem->Game(i).GetDirEntry().GetSaveDataOffsetInFile() << " + " << gc_mem->Game(i).GetDirEntry().GetBannerOffsetInSave() << ")"
					<< "\n\t\tIcon Formats:";
				for (icon_index = 0; icon_index < 8; icon_index++)
				{
					std::cout << "\n\t\t\t" << (icon_index + 1)
						<< ". Icon:\n\t\t\t\tIcon format: " << gc_mem->Game(i).GetDirEntry().GetIconFormatName(icon_index)
						<< "\n\t\t\t\tAnimation speed: " << gc_mem->Game(i).GetDirEntry().GetAnimationSpeedName(icon_index);
					//gc_mem->Game(i).StoreIconAsTGA((gc_mem->Game(i).GetDirEntry().GetGameCode() + "_icon_" + std::to_string(icon_index + 1)) + ".tga", icon_index);
				}
				std::cout << "\n\t\tIs save public: " << (gc_mem->Game(i).GetDirEntry().IsSavePublic() ? "yes" : "no")
					<< "\n\t\tIs copy disabled: " << (gc_mem->Game(i).GetDirEntry().CopyDisabled() ? "yes" : "no")
					<< "\n\t\tIs moving disabled: " << (gc_mem->Game(i).GetDirEntry().MovingDisabled() ? "yes" : "no")
					<< "\n\t\tCounter: " << ((int)(gc_mem->Game(i).GetDirEntry().GetCounter()))
					<< "\n\t\tSave size: " << gc_mem->Game(i).GetDirEntry().GetSaveSize()
					<< " bytes\n\t\tComment 1 offset: " << gc_mem->Game(i).GetDirEntry().GetComment1OffsetInFile() << " (" << gc_mem->Game(i).GetDirEntry().GetSaveDataOffsetInFile() << " + " << gc_mem->Game(i).GetDirEntry().GetComment1OffsetInSave() << ")"
					<< "\n\t\tComment 2 offset: " << gc_mem->Game(i).GetDirEntry().GetComment2OffsetInFile() << " (" << gc_mem->Game(i).GetDirEntry().GetSaveDataOffsetInFile() << " + " << gc_mem->Game(i).GetDirEntry().GetComment2OffsetInSave()
					<< ")\n\t\tComment 1: \"" << gc_mem->Game(i).GetComment1()
					<< "\"\n\t\tComment 2: \"" << gc_mem->Game(i).GetComment2()
					<< "\"";
				gc_mem->Game(i).StoreBannerAsTGA(gc_mem->Game(i).GetDirEntry().GetGameCode() + ".tga");
				gc_mem->Game(i).StoreIconsAsTGA(gc_mem->Game(i).GetDirEntry().GetGameCode() + "_icons.tga");
			}
			delete gc_mem;
		}
	}
	return 0;
}