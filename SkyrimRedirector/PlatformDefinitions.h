#pragma once

#ifdef SR_SPECIAL_EDITION
	#ifdef SR_GOG
		#define SR_FOLDER_SUFFIX_A " SPECIAL EDITION GOG"
		#define SR_FOLDER_SUFFIX_W L" SPECIAL EDITION GOG"

		#define SR_PLATFORM_IDENTIFIER_A "Special Edition GOG"
		#define SR_PLATFORM_IDENTIFIER_W L"Special Edition GOG"
	#else
		#define SR_FOLDER_SUFFIX_A " SPECIAL EDITION"
		#define SR_FOLDER_SUFFIX_W L" SPECIAL EDITION"

		#define SR_PLATFORM_IDENTIFIER_A "Special Edition Steam"
		#define SR_PLATFORM_IDENTIFIER_W L"Special Edition Steam"
//For VR, manual changes must be made by uncommenting below, in Redirections.c (4 lines), and Main.c (1 line). I may one day make a new solution configuration.
/*
		#define SR_FOLDER_SUFFIX_A " VR"
		#define SR_FOLDER_SUFFIX_W L" VR"

		#define SR_PLATFORM_IDENTIFIER_A "VR Steam"
		#define SR_PLATFORM_IDENTIFIER_W L"VR Steam"
*/
	#endif
#else
	#ifdef SR_GOG
		#error "Legendary Edition isn't supported by GOG'"
	#else
		#define SR_FOLDER_SUFFIX_A ""
		#define SR_FOLDER_SUFFIX_W L""

		#define SR_PLATFORM_IDENTIFIER_A "Legendary Edition Steam"
		#define SR_PLATFORM_IDENTIFIER_W L"Legendary Edition Steam"
	#endif
#endif
