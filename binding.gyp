{
  "targets":[
    {
      "target_name": "threads_a_gogo",
      "conditions": [
            ["OS==\"mac\"", {  
                              "sources": [ "src/threads_a_gogo.cpp","src/queues_a_gogo.cpp"],
                               "libraries": [],
                               "cflags": ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-O0", "-Wunused-macros"],
            }],
            ["OS==\"linux\"", {
                               "sources": [ "src/threads_a_gogo.cpp","src/queues_a_gogo.cpp"],
                               "libraries": [],
                               "cflags": ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-O0", "-Wunused-macros"],
            }],
            ["OS==\"win\"", {  
	                       "sources": [ "src/threads_a_gogo.cpp","src/queues_a_gogo.cpp", "src/pthreads.2/pthread.c"],
			                   "libraries": ["Ws2_32.lib"],
                         "cflags": ["/TP",'/J','/E'],
            }]
        ]
    }
  ]
}
