
struct Address
{                 
    string line1; 
    string line2; 
};

struct Organization
{
    struct Employee
    {                   
        string name;    
        string surname; 
    } emp;

    int type;            
    struct Address addr; 
};

int fact(int x)
{ 
    if (x <= 1)
    { 
        return 1;
    }
    else
        return fact(x - 1); 
};

int main(int argc, char *argv[])
{
    int x = 1; 

    int y = fact(10); 

    struct Organization org;  
    org.emp.name = "Dominik"; 
                              
}