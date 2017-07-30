#ifndef RESOURCE_H
#define RESOURCE_H


class Resource {	
	
public:

	//Resource() : locked(false) {}
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
	volatile bool locked = false;
};

#endif
