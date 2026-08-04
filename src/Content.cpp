#include "Content.h"
#include "UI.h"

#define ITEM(ID, CATEGORY, OWNER, NAME, PRICE) \
  { ID, ItemCategory::CATEGORY, ItemOwner::OWNER, NAME, PRICE, static_cast<uint8_t>((ID) & 0xFF) }

const ItemDef ITEMS[] = {
  ITEM(0, Character, StellaOnly, "DEFAULT PRINCESS", 0),
  ITEM(1, Character, StellaOnly, "ELSA", 160),
  ITEM(2, Character, StellaOnly, "ARIEL", 150),
  ITEM(3, Character, StellaOnly, "RAPUNZEL", 150),
  ITEM(4, Character, StellaOnly, "CINDERELLA", 160),
  ITEM(5, Character, StellaOnly, "MOANA", 150),
  ITEM(6, Character, StellaOnly, "RUMI", 180),
  ITEM(7, Character, StellaOnly, "MERMAID", 100),
  ITEM(8, Character, StellaOnly, "FAIRY", 100),
  ITEM(9, Character, StellaOnly, "UNICORN", 120),
  ITEM(10, Character, StellaOnly, "BUNNY", 80),
  ITEM(11, Character, StellaOnly, "KITTEN", 80),
  ITEM(12, Accessory, StellaOnly, "CROWN", 30),
  ITEM(13, Accessory, StellaOnly, "WAND", 25),
  ITEM(14, Accessory, StellaOnly, "TIARA", 30),
  ITEM(15, Accessory, StellaOnly, "RUMI PLAIT", 40),
  ITEM(16, Accessory, StellaOnly, "BUTTERFLY WINGS", 40),
  ITEM(17, Accessory, StellaOnly, "MAGIC NECKLACE", 35),
  ITEM(18, Accessory, StellaOnly, "FLOWER CROWN", 30),
  ITEM(19, Accessory, StellaOnly, "MERMAID TAIL", 40),
  ITEM(20, Accessory, StellaOnly, "SPARKLY SHOES", 35),
  ITEM(21, Accessory, StellaOnly, "STAR GLASSES", 25),
  ITEM(22, Accessory, StellaOnly, "SNOWFLAKE WAND", 35),
  ITEM(23, Accessory, StellaOnly, "SHELL NECKLACE", 30),
  ITEM(24, Accessory, StellaOnly, "LANTERN", 25),
  ITEM(25, Accessory, StellaOnly, "HUNTER MICROPHONE", 40),
  ITEM(26, Accessory, StellaOnly, "PRINCESS GLOVES", 30),
  ITEM(27, Clothing, StellaOnly, "RAINBOW DRESS", 60),
  ITEM(28, Clothing, StellaOnly, "PRINCESS DRESS", 55),
  ITEM(29, Clothing, StellaOnly, "ELSA ICE DRESS", 80),
  ITEM(30, Clothing, StellaOnly, "ARIEL MERMAID OUTFIT", 75),
  ITEM(31, Clothing, StellaOnly, "RAPUNZEL PURPLE DRESS", 75),
  ITEM(32, Clothing, StellaOnly, "CINDERELLA BALL GOWN", 80),
  ITEM(33, Clothing, StellaOnly, "MOANA ISLAND OUTFIT", 70),
  ITEM(34, Clothing, StellaOnly, "RUMI HUNTER OUTFIT", 80),
  ITEM(35, Clothing, StellaOnly, "FAIRY DRESS", 60),
  ITEM(36, Clothing, StellaOnly, "MERMAID OUTFIT", 65),
  ITEM(37, Clothing, StellaOnly, "PYJAMAS", 40),
  ITEM(38, Clothing, StellaOnly, "PARTY DRESS", 55),
  ITEM(39, Toy, StellaOnly, "CASTLE", 60),
  ITEM(40, Toy, StellaOnly, "TEA SET", 35),
  ITEM(41, Toy, StellaOnly, "MAGIC WAND", 40),
  ITEM(42, Toy, StellaOnly, "UNICORN TOY", 50),
  ITEM(43, Toy, StellaOnly, "DOLL", 40),
  ITEM(44, Toy, StellaOnly, "MUSIC BOX", 55),
  ITEM(45, Toy, StellaOnly, "JELLY BEAN JAR", 35),
  ITEM(46, Toy, StellaOnly, "SNOW GLOBE", 50),
  ITEM(47, Toy, StellaOnly, "MERMAID SHELL", 45),
  ITEM(48, Toy, StellaOnly, "BUTTERFLY TOY", 40),
  ITEM(49, Room, StellaOnly, "PRINCESS CASTLE", 0),
  ITEM(50, Room, StellaOnly, "PINK BEDROOM", 70),
  ITEM(51, Room, StellaOnly, "FLOWER GARDEN", 80),
  ITEM(52, Room, StellaOnly, "RAINBOW CLOUDS", 90),
  ITEM(53, Room, StellaOnly, "MERMAID LAGOON", 100),
  ITEM(54, Room, StellaOnly, "RUMI STAGE", 120),
  ITEM(55, Room, StellaOnly, "ELSA ICE PALACE", 120),
  ITEM(56, Room, StellaOnly, "RAPUNZEL TOWER", 110),
  ITEM(57, Room, StellaOnly, "MOANA ISLAND", 110),
  ITEM(58, Room, StellaOnly, "CANDY ROOM", 90),
  ITEM(59, Room, StellaOnly, "BUTTERFLY GARDEN", 80),
  ITEM(60, Character, HugoOnly, "SOCCER PLAYER", 0),
  ITEM(61, Character, HugoOnly, "DINO TRAINER", 100),
  ITEM(62, Character, HugoOnly, "T-REX", 120),
  ITEM(63, Character, HugoOnly, "RAPTOR", 110),
  ITEM(64, Character, HugoOnly, "SHARK", 100),
  ITEM(65, Character, HugoOnly, "SPIDER-MAN", 180),
  ITEM(66, Character, HugoOnly, "WOODY", 160),
  ITEM(67, Character, HugoOnly, "BUZZ LIGHTYEAR", 170),
  ITEM(68, Character, HugoOnly, "ASTRONAUT", 110),
  ITEM(69, Character, HugoOnly, "ROBOT", 100),
  ITEM(70, Character, HugoOnly, "EXPLORER", 90),
  ITEM(71, Character, HugoOnly, "FRANKIE", 120),
  ITEM(72, Accessory, HugoOnly, "SOCCER BOOTS", 30),
  ITEM(73, Accessory, HugoOnly, "SOCCER BALL", 25),
  ITEM(74, Accessory, HugoOnly, "CAPE", 30),
  ITEM(75, Accessory, HugoOnly, "SPIDER MASK", 40),
  ITEM(76, Accessory, HugoOnly, "DINOSAUR HAT", 35),
  ITEM(77, Accessory, HugoOnly, "SHARK FIN", 35),
  ITEM(78, Accessory, HugoOnly, "EXPLORER HAT", 25),
  ITEM(79, Accessory, HugoOnly, "ASTRO HELMET", 40),
  ITEM(80, Accessory, HugoOnly, "SHIELD", 35),
  ITEM(81, Accessory, HugoOnly, "SKATEBOARD", 35),
  ITEM(82, Accessory, HugoOnly, "GOALIE GLOVES", 30),
  ITEM(83, Accessory, HugoOnly, "COWBOY HAT", 30),
  ITEM(84, Accessory, HugoOnly, "SHERIFF BADGE", 25),
  ITEM(85, Accessory, HugoOnly, "SPACE WINGS", 40),
  ITEM(86, Accessory, HugoOnly, "DINO BACKPACK", 35),
  ITEM(87, Accessory, HugoOnly, "SHARK TEETH HAT", 35),
  ITEM(88, Clothing, HugoOnly, "SOCCER KIT", 55),
  ITEM(89, Clothing, HugoOnly, "DINOSAUR COSTUME", 70),
  ITEM(90, Clothing, HugoOnly, "SHARK COSTUME", 70),
  ITEM(91, Clothing, HugoOnly, "SPIDER SUIT", 80),
  ITEM(92, Clothing, HugoOnly, "ASTRONAUT SUIT", 75),
  ITEM(93, Clothing, HugoOnly, "WOODY COWBOY OUTFIT", 75),
  ITEM(94, Clothing, HugoOnly, "BUZZ SPACE SUIT", 80),
  ITEM(95, Clothing, HugoOnly, "EXPLORER OUTFIT", 55),
  ITEM(96, Clothing, HugoOnly, "ROBOT OUTFIT", 65),
  ITEM(97, Clothing, HugoOnly, "PYJAMAS", 40),
  ITEM(98, Toy, HugoOnly, "SOCCER BALL TOY", 35),
  ITEM(99, Toy, HugoOnly, "DINO FIGURES", 45),
  ITEM(100, Toy, HugoOnly, "SHARK TOY", 40),
  ITEM(101, Toy, HugoOnly, "ROBOT TOY", 50),
  ITEM(102, Toy, HugoOnly, "SKATEBOARD TOY", 45),
  ITEM(103, Toy, HugoOnly, "RACE CAR", 50),
  ITEM(104, Toy, HugoOnly, "JELLY BEAN JAR", 35),
  ITEM(105, Toy, HugoOnly, "COWBOY LASSO", 40),
  ITEM(106, Toy, HugoOnly, "SPACE ROCKET", 55),
  ITEM(107, Toy, HugoOnly, "GOAL SET", 60),
  ITEM(108, Toy, HugoOnly, "DINOSAUR EGG", 45),
  ITEM(109, Toy, HugoOnly, "TREASURE CHEST", 60),
  ITEM(110, Room, HugoOnly, "SOCCER STADIUM", 0),
  ITEM(111, Room, HugoOnly, "DINOSAUR JUNGLE", 100),
  ITEM(112, Room, HugoOnly, "VOLCANO", 100),
  ITEM(113, Room, HugoOnly, "SHARK OCEAN", 110),
  ITEM(114, Room, HugoOnly, "SPIDER CITY", 120),
  ITEM(115, Room, HugoOnly, "SPACE STATION", 110),
  ITEM(116, Room, HugoOnly, "WESTERN TOWN", 90),
  ITEM(117, Room, HugoOnly, "BUZZ SPACEPORT", 120),
  ITEM(118, Room, HugoOnly, "ROBOT LAB", 100),
  ITEM(119, Room, HugoOnly, "CANDY ROOM", 90),
  ITEM(120, Room, HugoOnly, "FRANKIE PARK", 80),
  ITEM(121, Treat, Both, "BAG OF JELLY BEANS", 10),
  ITEM(122, Treat, Both, "APPLE", 5),
  ITEM(123, Treat, Both, "CUPCAKE", 8),
  ITEM(124, Treat, Both, "ICE CREAM", 8),
  ITEM(125, Treat, Both, "COOKIE", 6),
  ITEM(126, Treat, Both, "STRAWBERRY", 5),
};

#undef ITEM

const int ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

const ItemDef* itemById(uint16_t id) {
  for (int i = 0; i < ITEM_COUNT; ++i) {
    if (ITEMS[i].id == id) return &ITEMS[i];
  }
  return nullptr;
}

#define ICON(ID) static_cast<uint8_t>(IconId::ID)

const TaskDef TASKS[] = {
  {0, "BRUSH TEETH", 10, ICON(Smiley)},
  {1, "GET DRESSED", 10, ICON(Shirt)},
  {2, "PACK AWAY TOYS", 15, ICON(Gift)},
  {3, "PUT SHOES AWAY", 5, ICON(Shoe)},
  {4, "HELP MAKE THE BED", 10, ICON(Bed)},
  {5, "HELP SET THE TABLE", 10, ICON(Plate)},
  {6, "READ A BOOK", 10, ICON(Book)},
  {7, "PLAY OUTSIDE", 10, ICON(Sun)},
  {8, "HELP FEED FRANKIE", 10, ICON(Paw)},
  {9, "WALK FRANKIE", 20, ICON(Paw)},
  {10, "DIRTY CLOTHES IN BASKET", 5, ICON(Shirt)},
  {11, "HELP CLEAN UP", 15, ICON(Broom)},
  {12, "CARRY OWN PLATE", 5, ICON(Plate)},
  {13, "PUT PYJAMAS AWAY", 5, ICON(Moon)},
  {14, "WATER A PLANT", 10, ICON(Water)},
};

const int TASK_COUNT = sizeof(TASKS) / sizeof(TASKS[0]);

const uint8_t ACTIVE_TASKS[] = {1, 9, 13, 0, 2};
const int ACTIVE_TASK_COUNT = sizeof(ACTIVE_TASKS) / sizeof(ACTIVE_TASKS[0]);

const DadMissionDef DAD_MISSIONS[] = {
  {0, "FIND SOMETHING RED", ICON(Circle), 20, 25},
  {1, "FIND SOMETHING BLUE", ICON(Circle), 20, 25},
  {2, "FIND A FEATHER", ICON(Feather), 25, 30},
  {3, "FIND THREE DIFFERENT LEAVES", ICON(Leaf), 30, 35},
  {4, "COUNT FIVE BIRDS", ICON(Feather), 25, 35},
  {5, "BUILD A TALL LEGO TOWER", ICON(Lego), 30, 40},
  {6, "DRAW A DINOSAUR", ICON(Dino), 25, 35},
  {7, "DRAW A PRINCESS", ICON(Crown), 25, 35},
  {8, "DO TEN BIG JUMPS", ICON(Jump), 30, 40},
  {9, "KICK A BALL TEN TIMES", ICON(Ball), 30, 40},
  {10, "GIVE FRANKIE A GENTLE PAT", ICON(Paw), 20, 30},
  {11, "FIND A CIRCLE SHAPE", ICON(Circle), 20, 25},
  {12, "FIND A TRIANGLE SHAPE", ICON(Triangle), 20, 25},
  {13, "TELL DAD YOUR FAVOURITE ANIMAL", ICON(Paw), 20, 30},
  {14, "TELL DAD YOUR FAVOURITE DINOSAUR", ICON(Dino), 20, 30},
  {15, "DANCE FOR TWENTY SECONDS", ICON(Note), 25, 35},
  {16, "HELP DAD CARRY SOMETHING", ICON(Home), 30, 40},
  {17, "FIND SOMETHING THAT SMELLS NICE", ICON(Flower), 25, 30},
  {18, "PRETEND TO BE A SHARK", ICON(Shark), 20, 30},
  {19, "PRETEND TO BE A PRINCESS", ICON(Crown), 20, 30},
  {20, "ROAR LIKE A DINOSAUR", ICON(Dino), 20, 30},
  {21, "WALK LIKE A T-REX", ICON(Dino), 25, 35},
  {22, "FIND SOMETHING BEGINNING WITH B", ICON(Book), 30, 40},
  {23, "FIND SOMETHING SOFT", ICON(Heart), 20, 30},
  {24, "FIND SOMETHING TALLER THAN YOU", ICON(ArrowR), 25, 35},
  {25, "FIND SOMETHING SMALLER THAN YOUR HAND", ICON(ArrowL), 25, 35},
  {26, "SING A SONG WITH DAD", ICON(Mic), 25, 35},
  {27, "FIND FIVE JELLY BEAN COLOURS", ICON(JellyBean), 30, 40},
  {28, "PUT THREE TOYS BACK", ICON(Gift), 25, 35},
  {29, "MAKE FRANKIE'S FUNNIEST FACE", ICON(Smiley), 20, 30},
};

const int DAD_MISSION_COUNT = sizeof(DAD_MISSIONS) / sizeof(DAD_MISSIONS[0]);

const DailyMissionDef DAILY_MISSIONS[] = {
  {0, MissionKind::MathsCorrect, 5, 5, "GET 5 MATHS CORRECT", ICON(Book), ItemOwner::HugoOnly},
  {1, MissionKind::PenaltyGoals, 2, 2, "SCORE 2 PENALTY GOALS", ICON(Ball), ItemOwner::HugoOnly},
  {2, MissionKind::FindDinosaurs, 3, 3, "FIND 3 DINOSAURS", ICON(Dino), ItemOwner::HugoOnly},
  {3, MissionKind::GetDressed, 0, 1, "GET DRESSED", ICON(Shirt), ItemOwner::HugoOnly},
  {4, MissionKind::WalkFrankie, 0, 1, "WALK FRANKIE", ICON(Paw), ItemOwner::HugoOnly},
  {5, MissionKind::DadMission, 0, 1, "DO 1 DAD MISSION", ICON(Star), ItemOwner::HugoOnly},
  {6, MissionKind::PlayGame, static_cast<uint8_t>(GameId::SharkSwim), 1, "PLAY SHARK GAME", ICON(Shark), ItemOwner::HugoOnly},
  {7, MissionKind::CompleteTask, 0, 1, "COMPLETE 1 TASK", ICON(Tick), ItemOwner::HugoOnly},
  {8, MissionKind::PopBubbles, 10, 10, "POP 10 BUBBLES", ICON(Circle), ItemOwner::StellaOnly},
  {9, MissionKind::FindColours, 3, 3, "FIND 3 COLOURS", ICON(Sparkle), ItemOwner::StellaOnly},
  {10, MissionKind::DoDance, 1, 1, "DO 1 PRINCESS DANCE", ICON(Note), ItemOwner::StellaOnly},
  {11, MissionKind::GetDressed, 0, 1, "GET DRESSED", ICON(Shirt), ItemOwner::StellaOnly},
  {12, MissionKind::WalkFrankie, 0, 1, "WALK FRANKIE", ICON(Paw), ItemOwner::StellaOnly},
  {13, MissionKind::DadMission, 0, 1, "DO 1 DAD MISSION", ICON(Star), ItemOwner::StellaOnly},
  {14, MissionKind::FeedBuddy, 0, 1, "FEED YOUR BUDDY", ICON(Apple), ItemOwner::StellaOnly},
  {15, MissionKind::CompleteTask, 0, 1, "COMPLETE 1 TASK", ICON(Tick), ItemOwner::StellaOnly},
  {16, MissionKind::PlayGame, static_cast<uint8_t>(GameId::RollerCoaster), 1, "PLAY ROLLER COASTER", ICON(Star), ItemOwner::Both},
  {17, MissionKind::PlayGame, static_cast<uint8_t>(GameId::BubblePop), 1, "PLAY BUBBLE POP", ICON(Circle), ItemOwner::Both},
  {18, MissionKind::FeedBuddy, 0, 1, "FEED YOUR BUDDY", ICON(Apple), ItemOwner::Both},
  {19, MissionKind::DadMission, 0, 1, "DO A DAD MISSION", ICON(Star), ItemOwner::Both},
};

const int DAILY_MISSION_COUNT = sizeof(DAILY_MISSIONS) / sizeof(DAILY_MISSIONS[0]);

const DinoDef DINOS[12] = {
  {0, "T-REX", false, false, false, false},
  {1, "TRICERATOPS", true, false, true, false},
  {2, "STEGOSAURUS", true, false, false, false},
  {3, "BRACHIOSAURUS", true, false, false, true},
  {4, "RAPTOR", false, false, false, false},
  {5, "PTERANODON", false, true, false, false},
  {6, "ANKYLOSAURUS", true, false, false, false},
  {7, "SPINOSAURUS", false, false, false, false},
  {8, "PARASAUROLOPHUS", true, false, false, false},
  {9, "CARNOTAURUS", false, false, true, false},
  {10, "DIPLODOCUS", true, false, false, true},
  {11, "PACHYCEPHALOSAURUS", true, false, false, false},
};

const char* JELLYBEAN_COLOURS[10] = {
  "RED", "BLUE", "GREEN", "YELLOW", "ORANGE",
  "PINK", "PURPLE", "WHITE", "BLACK", "RAINBOW",
};

const char* FRANKIE_FINDS[12] = {
  "STICK", "BONE", "BUTTERFLY", "SOCCER BALL", "PUDDLE", "FLOWER",
  "BIRD", "LEAF", "JELLY BEAN", "TOY DINOSAUR", "SHELL", "TREASURE CHEST",
};

const char* SHARK_NAMES[6] = {
  "BLUE SHARK", "HAMMERHEAD", "GREAT WHITE",
  "TIGER SHARK", "BABY SHARK", "ROBO SHARK",
};

namespace {

const char* DAD_BADGE_NAMES[6] = {
  "EXPLORER", "FINDER", "MOVER", "ARTIST", "ANIMAL FRIEND", "SUPER HELPER",
};

const char* GAME_BADGE_NAMES[8] = {
  "PENALTY STAR", "DINO EXPERT", "MATHS WHIZ", "COASTER KING",
  "DANCE STAR", "COLOUR GENIUS", "BUBBLE CHAMP", "SHARK HERO",
};

}

const char* collectibleName(CollectGroup g, uint8_t idx) {
  switch (g) {
    case CollectGroup::Dinosaur:
      return idx < 12 ? DINOS[idx].name : nullptr;
    case CollectGroup::JellyBean:
      return idx < 10 ? JELLYBEAN_COLOURS[idx] : nullptr;
    case CollectGroup::FrankieFind:
      return idx < 12 ? FRANKIE_FINDS[idx] : nullptr;
    case CollectGroup::DadBadge:
      return idx < 6 ? DAD_BADGE_NAMES[idx] : nullptr;
    case CollectGroup::GameBadge:
      return idx < 8 ? GAME_BADGE_NAMES[idx] : nullptr;
    case CollectGroup::Shark:
      return idx < 6 ? SHARK_NAMES[idx] : nullptr;
    default:
      return nullptr;
  }
}

#undef ICON
