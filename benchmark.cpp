#include <algorithm>
#include <cstring>
#include <cstdint>
#define main original_main
#include "Source.cpp"
#undef main

static void write_result(std::ostream& out, const _memory_region_header& h) {
 auto put=[&](const auto& x){out.write((const char*)&x,sizeof(x));};
 put(h.version_number); put(h.osu_headers.Mode); put(h.osu_headers.table);
 put(h.lines_skipped); put(h.stable_would_refuse);
 const auto n=h.ELEM_COUNT[MEM_object_header], nt=h.ELEM_COUNT[MEM_timing_point];
 put(n); put(nt);
 for(u32 i=0;i<nt;++i){const auto& t=h.get_timing_point()[i];put(t.time);put(t.beat_length);put(t.tick_beat_length);}
 for(u32 i=0;i<n;++i){const auto& a=h.get_object_header()[i];put(a.x);put(a.y);put(a.time);put(a.type);
  if(a.type&2){const auto& s=h.get_object_body()[i];put(s.length);put(s.slides);put(s.curve_type);
   const u64 count=s.point_start ? s.point_end-s.point_start : 0;put(count);
   for(u64 j=0;j<count;++j){put(s.point_start[j].x);put(s.point_start[j].y);}
  }
 }
}
int main(int argc,char** argv){
 if(argc<2)return 2;
 size_t limit=argc>2?std::stoull(argv[2]):20001;
 int passes=argc>3?std::stoi(argv[3]):5;
 size_t offset=argc>5?std::stoull(argv[5]):0;
 std::vector<std::filesystem::path> paths;
 for(auto& p:std::filesystem::directory_iterator(argv[1]))if(p.path().extension()==".osu")paths.push_back(p.path());
 std::sort(paths.begin(),paths.end());if(limit && paths.size()>limit)paths.resize(limit);
 std::vector<std::vector<char>> maps;
 for(auto& p:paths){size_t n=std::filesystem::file_size(p);std::vector<char> b(64+offset+n+129);std::ifstream f(p,std::ios::binary);if(!f.read(b.data()+64+offset,n))return 3;b[64+offset+n]='\n';maps.push_back(std::move(b));}
 if(!SetThreadAffinityMask(GetCurrentThread(),1ull<<2))return 4;
 auto* memory=create_memory_region();
 auto parse=[&](auto& b){parse_beatmap_from_memory(&memory->header,b.data()+64+offset,b.data()+b.size()-129);};
 std::ofstream dump;if(argc>4 && std::strcmp(argv[4],"-"))dump.open(argv[4],std::ios::binary);
 for(auto& b:maps){parse(b);if(dump.is_open())write_result(dump,memory->header);}
 for(int pass=0;pass<passes;++pass){auto start=std::chrono::steady_clock::now();for(int r=0;r<4;++r)for(auto& b:maps)parse(b);
 std::cout<<"pass="<<pass<<" ns/map="<<std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/(maps.size()*4)<<std::endl;}
 std::cout<<"maps="<<maps.size()<<std::endl;
}
