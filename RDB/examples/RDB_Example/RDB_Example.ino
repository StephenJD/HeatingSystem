#include <RDB.h>
#include <EEPROM.h>

using namespace RelationalDatabase;

int writer(int address, const void * data, int noOfBytes) {
  const unsigned char * byteData = static_cast<const unsigned char *>(data);
  for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
    EEPROM.update(address, *byteData);
  }
  return address;
}

int reader(int address, void * result, int noOfBytes) {
  uint8_t * byteData = static_cast<uint8_t *>(result);
  for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
    *byteData = EEPROM.read(address);
  }
  return address;
}

struct R_Office {
  int roomNumber;
  char description[20];
};

struct R_Person {
  RecordID office_ID;
  char name[10];
  int age;
  RecordID field(int) {
    return office_ID;
  }
};

struct R_Keyholder {
  RecordID personID;
  RecordID room_ID;
  RecordID field(int fieldIndex) {
    if (fieldIndex == 0) return personID; else return room_ID;
  }
};

enum {T_Offices, T_People, T_Keyholders, T_NoOfTables};
RDB<T_NoOfTables> db(0, 1024, writer, reader);

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  auto rs_office = db.createTable<R_Office>(5);
  auto rs_person = db.createTable<R_Person>(8);
  auto rs_keyholder = db.createTable<R_Keyholder>(16);

  R_Office offices[] = { { 100, "Reception" },
    { 102, "HR" },
    { 201, "Programmers" },
    { 202, "Salaries" },
    { 203, "Technical" }
  };
  rs_office.insertRecords(offices, sizeof(offices) / sizeof(R_Office));

  R_Person people[] = {
    { 0, "Amy", 35 }, // Reception
    { 0, "Betty", 45 },
    { 0, "Charles", 19 },
    { 1, "David", 22 }, // HR
    { 1, "Edward", 25 },
    { 2, "Freddy", 37 }, // Programmers
    { 2, "Gareth", 57 },
    { 2, "Harry", 29 },
    { 3, "Igor", 37 }, // Salaries
    { 4, "James", 21 }, // Technical
  };
  rs_person.insertRecords(people, sizeof(people) / sizeof(R_Person)); // initial capacity = 8 - needs to grow.

  R_Keyholder keyholders[] = {
    {1, 0},
    {4, 0},
    {3, 1},
    {8, 1},
    {5, 2},
    {6, 2},
    {7, 2},
    {8, 3},
    {9, 0},
    {9, 1},
    {9, 2},
    {9, 3},
    {9, 4}
  };
  rs_keyholder.insertRecords(keyholders, sizeof(keyholders) / sizeof(R_Keyholder));

  auto listofficesQuery = TableQuery(rs_office);
  auto office = listofficesQuery.execute();
  Serial.println("All Offices");
  while (listofficesQuery.more()) {
    Serial.print( "\t");
    Serial.print( office->rec().roomNumber);
    Serial.print( " : ");
    Serial.println( office->rec().description);
    office = listofficesQuery.next();
  }

  auto listPeopleQuery = TableQuery(rs_person);
  auto person = listPeopleQuery.execute();
  Serial.println("\nAll People");
  while (listPeopleQuery.more()) {
    Serial.print( "\t");
    Serial.print( person->rec().name);
    Serial.print( " Aged: ");
    Serial.println( person->rec().age);
    person = listPeopleQuery.next();
  }

  auto listPeopleInOfficeQuery = FilterQuery(rs_person, 1);
  office = listofficesQuery.execute();
  Serial.println("\nPeople in offices");
  while (listofficesQuery.more()) {
    Serial.print("\tPeople in ");
    Serial.println( office->rec().description);
    auto person = listPeopleInOfficeQuery.execute(office->id());
    while (listPeopleInOfficeQuery.more()) {
      Serial.print( "\t\t");
      Serial.print( person->rec().name);
      Serial.print( " Aged: ");
      Serial.println( person->rec().age);
      person = listPeopleInOfficeQuery.next(office->id());
    }
    office = listofficesQuery.next();
  }

  auto listKeyholdersQuery = JoinQuery(rs_keyholder, rs_person, 1, 0);
  Serial.println("\nKeyholders for offices");
  office = listofficesQuery.execute();
  while (listofficesQuery.more()) {
    Serial.print("\tKeyholders for ");
    Serial.println( office->rec().description);
    auto person = listKeyholdersQuery.execute(office->id());
    while (listKeyholdersQuery.more()) {
      Serial.print( "\t\t");
      Serial.print( person->rec().name);
      Serial.print( " Aged: ");
      Serial.println( person->rec().age);
      person = listKeyholdersQuery.next(office->id());
    }
    office = listofficesQuery.next();
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
