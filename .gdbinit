target extended-remote localhost:3333
define reload
    monitor reset halt
    load
end 
