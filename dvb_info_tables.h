/* 
 * tv_grab_dvb - (c) Mark Bryars 2004
 * God bless vim and macros, this would have taken forever to format otherwise.
 * 
 */

#include "lookup.h"

static struct lookup_table description_table[]={ 
	{ 0x10, "Movie / Drama" },
	{ 0x11, "Movie - detective/thriller" },
	{ 0x12, "Movie - adventure/western/war" },
	{ 0x13, "Movie - science fiction/fantasy/horror" },
	{ 0x14, "Movie - comedy" },
	{ 0x15, "Movie - soap/melodrama/folkloric" },
	{ 0x16, "Movie - romance" },
	{ 0x17, "Movie - serious/classical/religious/historical movie/drama" },
	{ 0x18, "Movie - adult movie/drama" },
	
	{ 0x20, "News / Current Affairs" },
	{ 0x21, "news/weather report" },
	{ 0x22, "news magazine" },
	{ 0x23, "documentary" },
	{ 0x24, "discussion/interview/debate" },
	
	{ 0x30, "Show / Game Show" },
	{ 0x31, "game show/quiz/contest" },
	{ 0x32, "variety show" },
	{ 0x33, "talk show" },
	
	{ 0x40, "Sports" },
	{ 0x41, "special events (Olympic Games, World Cup etc.)" },
	{ 0x42, "sports magazines" },
	{ 0x43, "football/soccer" },
	{ 0x44, "tennis/squash" },
	{ 0x45, "team sports (excluding football)" },
	{ 0x46, "athletics" },
	{ 0x47, "motor sport" },
	{ 0x48, "water sport" },
	{ 0x49, "winter sports" },
	{ 0x4A, "equestrian" },
	{ 0x4B, "martial sports" },
	
	{ 0x50, "Childrens / Youth" },
	{ 0x51, "pre-school children's programmes" },
	{ 0x52, "entertainment programmes for 6 to14" },
	{ 0x53, "entertainment programmes for 10 to 16" },
	{ 0x54, "informational/educational/school programmes" },
	{ 0x55, "cartoons/puppets" },
	
	{ 0x60, "Music / Ballet / Dance" },
	{ 0x61, "rock/pop" },
	{ 0x62, "serious music/classical music" },
	{ 0x63, "folk/traditional music" },
	{ 0x64, "jazz" },
	{ 0x65, "musical/opera" },
	{ 0x66, "ballet" },

	{ 0x70, "Arts / Culture" },
	{ 0x71, "performing arts" },
	{ 0x72, "fine arts" },
	{ 0x73, "religion" },
	{ 0x74, "popular culture/traditional arts" },
	{ 0x75, "literature" },
	{ 0x76, "film/cinema" },
	{ 0x77, "experimental film/video" },
	{ 0x78, "broadcasting/press" },
	{ 0x79, "new media" },
	{ 0x7A, "arts/culture magazines" },
	{ 0x7B, "fashion" },
	
	{ 0x80, "Social / Policical / Economics" },
	{ 0x81, "magazines/reports/documentary" },
	{ 0x82, "economics/social advisory" },
	{ 0x83, "remarkable people" },
	
	{ 0x90, "Education / Science / Factual" },
	{ 0x91, "nature/animals/environment" },
	{ 0x92, "technology/natural sciences" },
	{ 0x93, "medicine/physiology/psychology" },
	{ 0x94, "foreign countries/expeditions" },
	{ 0x95, "social/spiritual sciences" },
	{ 0x96, "further education" },
	{ 0x97, "languages" },
	
	{ 0xA0, "Leisure / Hobbies" },
	{ 0xA1, "tourism/travel" },
	{ 0xA2, "handicraft" },
	{ 0xA3, "motoring" },
	{ 0xA4, "fitness & health" },
	{ 0xA5, "cooking" },
	{ 0xA6, "advertizement/shopping" },
	{ 0xA7, "gardening" },
	
	// Special
	{ 0xB0, "Original Language" },
	{ 0xB1, "black & white" },
	{ 0xB2, "unpublished" },
	{ 0xB3, "live broadcast" },

	// UK Freeview custom id
	{ 0xF0, "Drama" },
	
	{ -1, NULL }	
};

static struct lookup_table aspect_table[] = {
        {0, "4:3"},   // 4/3  
        {1, "16:9"},  // 16/9 WITH PAN VECTORS
        {2, "16:9"},  // 16/9 WITHOUT
        {3, "2.21:1"},  // >16/9 or 2.21/1 XMLTV no likey
	{-1, NULL }	
};

static struct lookup_table audio_table[] = {
	{0x01, "mono" },      //single mono
	{0x02, "mono" },	  //dual mono - stereo??
	{0x03, "stereo" },
	{0x05, "surround" },
	{0x04, "x-multilingual"}, // multilingual/channel
	{0x40, "x-visuallyimpared"}, // visual impared sound
        {0x41, "x-hardofhearing"}, // hard hearing sound
	{-1, NULL }
};


