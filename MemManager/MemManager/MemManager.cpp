#include "MemManager.h"
/////////////////////////////////////////////
// Globalne pole running
PROTEZA::PCB *running;
void PROTEZA::showProcessList() {
	for (auto x : process_list) {
		std::cout << "PROCESS: " << x.name << " PID " << x.PID << " LR: " << x.LR << "\n Page Table: "; 
		MemoryManager::showPageTable(x.page_table);
	}
	std::cout << std::endl;
};

void PROTEZA::showRunning() {
	std::cout << "RUNNING PROCESS: " << running->name << " PID " << running->PID << " LR: " << running->LR << " Page Table: ";
	MemoryManager::showPageTable(running->page_table);
	std::cout << std::endl;
};

void PROTEZA::selectRunning(int PID) {
	std::cout << "Zmiana procesu running" << std::endl;

	for (int i = 0; i < process_list.size(); i++) {
		if (PID == process_list[i].PID) {
			running = &process_list[i];
			break;
		}
	}
};

void PROTEZA::GO(MemoryManager mm) {
	std::cout << "Wykonany rozkaz: ";
	std::string rozkaz = mm.Get(running->LR);
	std::cout << rozkaz << " ";;
	running->LR += rozkaz.size()+1;

	while (true) {
		rozkaz = mm.Get(running->LR);
		if (rozkaz.size() == 0 || !isdigit(rozkaz[0]))
			break;
		else
			std::cout << rozkaz<<" ";
		running->LR += rozkaz.size() + 1;
	}
	std::cout <<std::endl;
};

/////////////////////////////////////////////

void MemoryManager::start() {
	//wypelniam ram spacjami
	for (int i = 0; i < 128; i++) {
		RAM[i] = ' ';
	}
	std::string data = "JP 0";
	std::vector<Page> Pvec{ Page(data) };
	PageFile.insert(std::make_pair(0, Pvec));
}

void MemoryManager::showFrameList() {
	std::cout << "\t FREE \t PAGE \t PID" << std::endl;
	for (int i = 0; i < 8; i++) {
		std::cout << "FRAME " << i << ":  " << Frames[i].free << " \t " << Frames[i].pageN << " \t " << Frames[i].PID << std::endl;
	}
}

void MemoryManager::showPMemory(int start, int amount) {
	if (start + amount > 128)
		std::cout << "Nie mozna przeczytac pamieci podano zly zakres" << std::endl;
	else {
		std::cout << "Pamiec fizyczna od adresu " << start << " do adresu " << start + amount << std::endl;
		for (int i = start; i < amount + start; i++) {
			//nowa linia po 16 znakach
			if (i % 16 == 0 && i != 0)
				std::cout << std::endl;
			//zamieniamy spacje na _ dla ulatwienia odczytu
			if (RAM[i] != ' ')
				std::cout << RAM[i];
			else
				std::cout << '_';

		}
		std::cout << std::endl;
	}
}

void MemoryManager::showPMemory() {
	for (int i = 0; i < 128; i++) {
		if (i % 16 == 0)
			std::cout << std::endl << "FRAME " << i / 16 << " ";
		if (RAM[i] != ' ')
			std::cout << RAM[i];
		else
			std::cout << '_';

	}
	std::cout << std::endl;
}

void MemoryManager::showPageTable(std::vector<PageTableData> *page_table) {
	//print running->PID
	for (int i = 0; i < page_table->size(); i++)
		std::cout << page_table->at(i).frame << " " << page_table->at(i).bit << std::endl;
}

void MemoryManager::ShowPageFile() {
	for (auto x : PageFile) {
		std::cout << "PROCESS ID: " << x.first << std::endl;
		for (int i = 0; i < x.second.size(); i++) {
			x.second.at(i).print();
		}
	}
}

void MemoryManager::ShowLRUList() {
	for (auto it : LRU) {
		std::cout << it << " ";
	}
}

void MemoryManager::FrameOrder(int Frame) {
	if (Frame > 7)
		return;

	for (auto it = LRU.begin(); it != LRU.end(); it++) {
		if (*it == Frame) {
			LRU.erase(it);
			break;
		}
	}
	LRU.push_front(Frame);
}

int MemoryManager::seekForFreeFrame() {
	int Frame = -1;
	for (int i = 0; i < Frames.size(); i++) {
		if (Frames[i].free == 1) {
			Frame = i;
			break;
		}
	}
	return Frame;
}

int MemoryManager::LoadtoMemory(Page page, int pageN, int PID, std::vector<PageTableData> *page_table) {
	int n = 0;
	int Frame = -1;
	Frame = seekForFreeFrame();
	if (Frame == -1)//Nie ma wolnych 
		Frame = SwapPages(page_table, pageN, PID);
	int end = Frame * 16 + 16;

	for (int i = Frame * 16; i < end; i++) {
		RAM[i] = page.data[n];
		n++;
	}

	//Zmieniamy wartosci w tablicy stronic
	page_table->at(pageN).bit = 1;
	page_table->at(pageN).frame = Frame;

	//Uzylismy ramki wiec wedruje na poczatek listy LRU oraz oznaczamy ja jako zaj�t�
	FrameOrder(Frame);
	Frames[Frame].free = 0;
	Frames[Frame].pageN = pageN;
	Frames[Frame].page_table = page_table;
	Frames[Frame].PID = PID;

	return Frame;
}

int MemoryManager::LoadProgram(std::string path, int mem, int PID) {
	double pages = ceil((double)mem / 16);
	std::fstream file; // zmienna pliku
	std::string str; // zmienna pomocnicza
	std::string program; // caly program w jedym lancuchu
	std::vector<Page> pagevec; // vector zawierajacy stronice do dodania
	file.open(path, std::ios::in);

	//std::cout << "Czytam program o nazwie "<<path<<std::endl;
	while (std::getline(file, str)) {
		//dodaje spacje zamiast konca lini
		if (!file.eof())
			str += " ";
		program += str;
	}
	str.clear();

	//Dziele program na stronice
	for (int i = 0; i < program.size(); i++) {
		str += program[i];
		//toworzymy stronice
		if (str.size() == 16) {
			pagevec.push_back(Page(str));
			str.clear();
		}
	}

	//ostatnia stronica
	if (str.size() != 0)
		pagevec.push_back(Page(str));
	str.clear();

	//Jezeli przydzielono za malo pamieci wywalam blad
	if (mem < 16 * pagevec.size()) {
		std::cout << "Przydzielono za malo pamieci programowi";
		return -1;
	}

	//Sprawdzam czy program nie potrzebuje dodatkowej pustej stronicy (wolne miejsce w programie)
	for (int i = pagevec.size(); i < pages; i++)
		pagevec.push_back(str);

	//Dodanie stronic do pliku wymiany
	PageFile.insert(std::make_pair(PID, pagevec));

	return 1;
}

int MemoryManager::SwapPages(std::vector<PageTableData> *page_table, int pageN, int PID) {
	//*it numer ramki ktora jest ofiara
	auto it = LRU.end(); it--;
	int Frame = *it;
	// Przepisuje zawartosc z ramki ofiary do Pliku wymiany
	for (int i = Frame * 16; i < Frame * 16 + 16; i++) {
		PageFile[Frames[Frame].PID][Frames[Frame].pageN].data[i - (Frame * 16)] = RAM[i];
	}

	//Zmieniam wartosci w tablicy stronic ofiary
	Frames[Frame].page_table->at(Frames[Frame].pageN).bit = 0;
	Frames[Frame].page_table->at(Frames[Frame].pageN).frame = -1;

	return Frame;
}

std::vector<MemoryManager::PageTableData>* MemoryManager::createPageTable(int mem, int PID) {
	double pages = ceil((double)mem / 16);
	int Frame = -1;
	std::vector<PageTableData> *page_table = new std::vector<PageTableData>();

	//Tworzenie tablicy
	for (int i = 0; i < pages; i++)
		page_table->push_back(PageTableData(-1, 0));

	//Zaladowanie pierwszej stronicy do pamieci fizycznej
	LoadtoMemory(PageFile.at(PID).at(0), 0, PID, page_table);

	return page_table;
}

std::string MemoryManager::Get(int LR) {
	//string do wyslania
	std::string order;
	bool koniec = false;
	int Frame = -1;
	int stronica = LR / 16;

	// przekroczenie zakresu dla tego procesu
	if (running->page_table->size() <= stronica) {
		std::cout << "Przekroczenie zakresu";
		koniec = true;
		order = "ERROR";
	}
	while (!koniec) {
		stronica = LR / 16; //stronica ktora musi znajdowac sie w pamieci
		// koniec programu
		if (running->page_table->size() <= stronica) {
			koniec = true;
			break;
		}

		//brak stronicy w pamieci operacyjnej
		if (running->page_table->at(stronica).bit == 0)
			LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);
		
		//stronica w pamieci operacyjnej
		if (running->page_table->at(stronica).bit == 1) {
			Frame = running->page_table->at(stronica).frame;		//Ramka w ktorej pracuje
			FrameOrder(Frame);									//uzywam ramki wiec zmieniam jej pozycje na liscie LRU
			if (RAM[Frame * 16 + LR - (16 * stronica)] == ' ')	//Czytam do napotkania spacji
				koniec = true;
			else
				order += RAM[Frame * 16 + LR - (16 * stronica)];
		}
		LR++;
	}
	return order;
}

void MemoryManager::Remove(int PID) {
	for (int i = 0; i < Frames.size(); i++) {
		if (Frames.at(i).PID == PID) { // Zerowanie pamieci RAM
			for (int j = i * 16; j < i * 16 + 16; j++) {
				RAM[j] = ' ';
			}

			Frames.at(i).free = 1; // Komorka znowu wolna
			Frames.at(i).pageN = -1;
			Frames.at(i).PID = -1;
			PageFile.erase(PID);
			//delete Frames.at(i).page_table; // usuwanie tablicy stronic dla procesu
		}
	}
}

int MemoryManager::Write(int adress, int PID, char ch, std::vector<PageTableData> *page_table) {
	int Frame = 1;
	int stronica = adress / 16;

	std::string str = PageFile[PID].at(stronica).data;
	if (str[(stronica * 16 - adress)* -1] == ' ') {
		//sprawdzenie czy stronica jest w pamieci
		if (page_table->at(stronica).bit == 1)
			Frame = page_table->at(stronica).frame;
		else
			Frame = LoadtoMemory(PageFile[PID].at(stronica), stronica, PID, page_table);

		//wpisanie do pamieci
		RAM[Frame * 16 + adress - (16 * stronica)] = ch;
	}
	else
		return -1;
}

int main() {
	//PROCESS INIT
	MemoryManager MM;
	PROTEZA proteza;
	MM.start();
	proteza.process_list.push_back(PROTEZA::PCB(0, 0, "INIT", MM.createPageTable(4, 0)));
	running = &proteza.process_list[0];

	MM.LoadProgram("program.txt", 48, 1);
	proteza.process_list.push_back(PROTEZA::PCB(1, 0, "p1", MM.createPageTable(48, 1)));
	running = &proteza.process_list[1];

	proteza.GO(MM);
	proteza.showRunning();
	proteza.showProcessList();
	MM.ShowPageFile();
	MM.showPMemory();
	MM.showFrameList();

	system("PAUSE");
}
