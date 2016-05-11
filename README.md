![logo](https://cloud.githubusercontent.com/assets/3276768/15173727/5f5608f8-1790-11e6-895f-870d7c199d60.png)


<img align="left" width="366" alt="screen shot 2016-05-11 at 15 54 58" src="https://cloud.githubusercontent.com/assets/3276768/15173809/cc4a00ea-1790-11e6-999d-611bb5123a53.png">

</br></br></br>
Small, and basic http server, written in full **C** 😾    🐕.
Use the **kevent** for async socket. Available for **OSX**, or **Free BSD**.


</br></br>
#Installation

Use CMake to generate a Makefile, and the compile the source.
```sh
cmake .
make
```

#How to use

```C
//Include the rocko public API
#include "rocko.h"

//Create a function which defines your route.
//Keep in mind the prototype has to be this one.
//You get the request informations in parameters with the body.
struct response welcome(struct request req) {
    //Here you build your response.
    //Following this schema :
    //{response_code, body}
    return (struct response){R_200, "Welcome !"};
}

int main(int argc, char **argv) {
    //Initalise rocko server has to be call before everything
    rocko_init();
    
    //Add your routes here with your method, the request_uri, and your function
    rocko_add_route("GET", "/welcome", welcome);
    
    //Start and run the server with your port
    rocko_start(8080);
    return 0
}
```
