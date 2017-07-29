#ifndef RESSOURCE_H
#define RESSOURCE_H


class Ressource {	
	
public:

	//Ressource() : locked(false) {}
	//seems to work without the constructor..
	
	bool get_lock_value(){ return locked;}

	void lock(){
		while (true){
			if(!locked){
				cli();
				if(!locked){
					locked=true;
					sei();
					return;
				}
				sei();
			}
		}
		//sleep();
	}	
	
	void unlock(){
		cli();
		locked=false;
		sei();
	}


private:
	bool locked = false;
};

#endif
