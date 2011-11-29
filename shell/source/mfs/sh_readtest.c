/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved                       
*
* Copyright (c) 2004-2008 Embedded Access Inc.;
* All Rights Reserved
*
* Copyright (c) 1989-2008 ARC International;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: sh_readtest.c$
* $Version : 3.7.15.0$
* $Date    : Feb-16-2011$
*
* Comments:
*
*   This file contains the source for an MFS shell function.
*
*END************************************************************************/

#include <string.h>
#include <mqx.h>
#include <fio.h>
#include <shell.h>
#include <mfs.h>
#include <sh_mfs.h>
#include "sh_prv.h"

#if SHELLCFG_USES_MFS

const char MFS_File_test_data[] = 
"3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342"
"11706798214808651328230664709384460955058223172535940812848111745028410270193852110555964462294"
"89549303819644288109756659334461284756482337867831652712019091456485669234603486104543266482133"
"93607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466"
"52138414695194151160943305727036575959195309218611738193261179310511854807446237996274956735188"
"57527248912279381830119491298336733624406566430860213949463952247371907021798609437027705392171"
"76293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224"
"95343014654958537105079227968925892354201995611212902196086403441815981362977477130996051870721"
"13499999983729780499510597317328160963185950244594553469083026425223082533446850352619311881710"
"10003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957"
"78185778053217122680661300192787661119590921642019893809525720106548586327886593615338182796823"
"03019520353018529689957736225994138912497217752834791315155748572424541506959508295331168617278"
"55889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955"
"59619894676783744944825537977472684710404753464620804668425906949129331367702898915210475216205"
"69660240580381501935112533824300355876402474964732639141992726042699227967823547816360093417216"
"41219924586315030286182974555706749838505494588586926995690927210797509302955321165344987202755"
"96023648066549911988183479775356636980742654252786255181841757467289097777279380008164706001614"
"52491921732172147723501414419735685481613611573525521334757418494684385233239073941433345477624"
"16862518983569485562099219222184272550254256887671790494601653466804988627232791786085784383827"
"96797668145410095388378636095068006422512520511739298489608412848862694560424196528502221066118"
"63067442786220391949450471237137869609563643719172874677646575739624138908658326459958133904780"
"27590099465764078951269468398352595709825822620522489407726719478268482601476990902640136394437"
"45530506820349625245174939965143142980919065925093722169646151570985838741059788595977297549893"
"01617539284681382686838689427741559918559252459539594310499725246808459872736446958486538367362"
"22626099124608051243884390451244136549762780797715691435997700129616089441694868555848406353422"
"07222582848864815845602850601684273945226746767889525213852254995466672782398645659611635488623"
"05774564980355936345681743241125150760694794510965960940252288797108931456691368672287489405601"
"01503308617928680920874760917824938589009714909675985261365549781893129784821682998948722658804"
"85756401427047755513237964145152374623436454285844479526586782105114135473573952311342716610213"
"59695362314429524849371871101457654035902799344037420073105785390621983874478084784896833214457"
"13868751943506430218453191048481005370614680674919278191197939952061419663428754440643745123718"
"19217999839101591956181467514269123974894090718649423196156794520809514655022523160388193014209"
"37621378559566389377870830390697920773467221825625996615014215030680384477345492026054146659252"
"01497442850732518666002132434088190710486331734649651453905796268561005508106658796998163574736"
"38405257145910289706414011097120628043903975951567715770042033786993600723055876317635942187312"
"51471205329281918261861258673215791984148488291644706095752706957220917567116722910981690915280"
"17350671274858322287183520935396572512108357915136988209144421006751033467110314126711136990865"
"85163983150197016515116851714376576183515565088490998985998238734552833163550764791853589322618"
"54896321329330898570642046752590709154814165498594616371802709819943099244889575712828905923233"
"26097299712084433573265489382391193259746366730583604142813883032038249037589852437441702913276"
"56180937734440307074692112019130203303801976211011004492932151608424448596376698389522868478312"
"35526582131449576857262433441893039686426243410773226978028073189154411010446823252716201052652"
"27211166039666557309254711055785376346682065310989652691862056476931257058635662018558100729360"
"65987648611791045334885034611365768675324944166803962657978771855608455296541266540853061434443"
"18586769751456614068007002378776591344017127494704205622305389945613140711270004078547332699390"
"81454664645880797270826683063432858785698305235808933065757406795457163775254202114955761581400"
"25012622859413021647155097925923099079654737612551765675135751782966645477917450112996148903046"
"39947132962107340437518957359614589019389713111790429782856475032031986915140287080859904801094"
"12147221317947647772622414254854540332157185306142288137585043063321751829798662237172159160771"
"66925474873898665494945011465406284336639379003976926567214638530673609657120918076383271664162"
"74888800786925602902284721040317211860820419000422966171196377921337575114959501566049631862947"
"26547364252308177036751590673502350728354056704038674351362222477158915049530984448933309634087"
"80769325993978054193414473774418426312986080998886874132604721569516239658645730216315981931951"
"67353812974167729478672422924654366800980676928238280689964004824354037014163149658979409243237"
"89690706977942236250822168895738379862300159377647165122893578601588161755782973523344604281512"
"62720373431465319777741603199066554187639792933441952154134189948544473456738316249934191318148"
"09277771038638773431772075456545322077709212019051660962804909263601975988281613323166636528619"
"32668633606273567630354477628035045077723554710585954870279081435624014517180624643626794561275"
"31813407833033625423278394497538243720583531147711992606381334677687969597030983391307710987040"
"85913374641442822772634659470474587847787201927715280731767907707157213444730605700733492436931"
"13835049316312840425121925651798069411352801314701304781643788518529092854520116583934196562134"
"91434159562586586557055269049652098580338507224264829397285847831630577775606888764462482468579"
"26039535277348030480290058760758251047470916439613626760449256274204208320856611906254543372131"
"53595845068772460290161876679524061634252257719542916299193064553779914037340432875262888963995"
"87947572917464263574552540790914513571113694109119393251910760208252026187985318877058429725916"
"77813149699009019211697173727847684726860849003377024242916513005005168323364350389517029893922"
"33451722013812806965011784408745196012122859937162313017114448464090389064495444006198690754851"
"60263275052983491874078668088183385102283345085048608250393021332197155184306354550076682829493"
"04137765527939751754613953984683393638304746119966538581538420568533862186725233402830871123282"
"78921250771262946322956398989893582116745627010218356462201349671518819097303811980049734072396"
"10368540664319395097901906996395524530054505806855019567302292191393391856803449039820595510022"
"63535361920419947455385938102343955449597783779023742161727111723643435439478221818528624085140"
"06660443325888569867054315470696574745855033232334210730154594051655379068662733379958511562578"
"43229882737231989875714159578111963583300594087306812160287649628674460477464915995054973742562"
"69010490377819868359381465741268049256487985561453723478673303904688383436346553794986419270563"
"87293174872332083760112302991136793862708943879936201629515413371424892830722012690147546684765"
"35761647737946752004907571555278196536213239264061601363581559074220202031872776052772190055614"
"84255518792530343513984425322341576233610642506390497500865627109535919465897514131034822769306"
"24743536325691607815478181152843667957061108615331504452127473924544945423682886061340841486377"
"67009612071512491404302725386076482363414334623518975766452164137679690314950191085759844239198"
"62916421939949072362346468441173940326591840443780513338945257423995082965912285085558215725031"
"07125701266830240292952522011872676756220415420516184163484756516999811614101002996078386909291"
"60302884002691041407928862150784245167090870006992821206604183718065355672525325675328612910424"
"87761825829765157959847035622262934860034158722980534989650226291748788202734209222245339856264"
"76691490556284250391275771028402799806636582548892648802545661017296702664076559042909945681506"
"52653053718294127033693137851786090407086671149655834343476933857817113864558736781230145876871"
"26603489139095620099393610310291616152881384379099042317473363948045759314931405297634757481193"
"56709110137751721008031559024853090669203767192203322909433467685142214477379393751703443661991"
"04033751117354719185504644902636551281622882446257591633303910722538374218214088350865739177150"
"96828874782656995995744906617583441375223970968340800535598491754173818839994469748676265516582"
"76584835884531427756879002909517028352971634456212964043523117600665101241200659755851276178583"
"82920419748442360800719304576189323492292796501987518721272675079812554709589045563579212210333"
"46697499235630254947802490114195212382815309114079073860251522742995818072471625916685451333123"
"94804947079119153267343028244186041426363954800044800267049624820179289647669758318327131425170"
"29692348896276684403232609275249603579964692565049368183609003238092934595889706953653494060340"
"21665443755890045632882250545255640564482465151875471196218443965825337543885690941130315095261"
"79378002974120766514793942590298969594699556576121865619673378623625612521632086286922210327488"
"92186543648022967807057656151446320469279068212073883778142335628236089632080682224680122482611"
"77185896381409183903673672220888321513755600372798394004152970028783076670944474560134556417254"
"37090697939612257142989467154357846878861444581231459357198492252847160504922124247014121478057"
"34551050080190869960330276347870810817545011930714122339086639383395294257869050764310063835198"
"34389341596131854347546495569781038293097164651438407007073604112373599843452251610507027056235"
"26601276484830840761183013052793205427462865403603674532865105706587488225698157936789766974220"
"57505968344086973502014102067235850200724522563265134105592401902742162484391403599895353945909"
"44070469120914093870012645600162374288021092764579310657922955249887275846101264836999892256959"
"6881592056001016552563756";

const uint_32 MFS_File_test_data_size = sizeof(MFS_File_test_data)-1;


/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    :   Shell_read_test
* Returned Value   :  int_32 error code
* Comments  :  Reads from a file .
*
*END*---------------------------------------------------------------------*/
#ifndef min
   #define min(a,b) ((a)<(b)?(a):(b))
#endif

int_32  Shell_read_test(int_32 argc, char_ptr argv[] )
{ 
   boolean           print_usage, shorthelp = FALSE;
   int_32            return_code = SHELL_EXIT_SUCCESS;
   MQX_FILE_PTR      fd; 
   char_ptr          read_buffer = NULL;
   uint_32           test_size = MFS_File_test_data_size;
   uint_32           read_size = 1,num,size,bytes_read,i,p=0;
   SHELL_CONTEXT_PTR    shell_ptr = Shell_get_context( argv );
   char_ptr             abs_path = NULL;
   int_32               error = 0;      

   print_usage = Shell_check_help_request(argc, argv, &shorthelp );

   if (!print_usage)  {
      if (argc != 2) {
         printf("Error, invalid number of parameters\n");
         return_code = SHELL_EXIT_ERROR;
         print_usage=TRUE;
      } else  {
         if (MFS_alloc_path(&abs_path) != MFS_NO_ERROR) {
            printf("Error, unable to allocate memory for paths\n" );
            return_code = SHELL_EXIT_ERROR;
         }
         else
         {
            error = _io_rel2abs(abs_path,shell_ptr->CURRENT_DIR,(char *) argv[1],PATHNAME_SIZE,shell_ptr->CURRENT_DEVICE_NAME);
            if(!error)
            {
               fd = fopen(abs_path, "w");
               write(fd, (pointer)MFS_File_test_data, test_size);
               fclose(fd);
            
               fd = fopen(abs_path, "r");
               read_buffer = _mem_alloc(test_size + 1);
               _mem_zero (read_buffer, test_size + 1);
            }
            if (fd && read_buffer && !error)  {
               for (read_size=1;read_size<test_size;read_size++) {
                  printf("Test Iteration #%d ", read_size);
                  fseek(fd,0,IO_SEEK_SET);
                  bytes_read = 0;
                  while ( bytes_read<test_size) {
                     size = min(read_size,test_size-bytes_read);
                     num = read(fd, &read_buffer[bytes_read],size);
                     if (num) {
                        bytes_read+=num;
                     } else {
                        printf("Read failed, offset = %d", bytes_read);
                       break;
                     }
                  } 
                  for (i = 0; i < test_size; i++)
                  {
                     if (MFS_File_test_data[i] != read_buffer[i]) {
                        printf("Failed %d. %d != %d\n", i, MFS_File_test_data[i], read_buffer[i]);
                        p++;
                        break;
                     }
                  }
                  if (i == test_size)
                  {
                     printf("Passed, errors: %d\n", p);
                  }
               } 
            }
            else
            {
               printf("Error, unable open file: %s\n", argv[1]);
            }
            if (fd) fclose(fd);
            if (read_buffer) _mem_free(read_buffer);
            MFS_free_path(abs_path);
         }
      }
   }
   
   if (print_usage)  {
      if (shorthelp)  {
         printf("%s <filename> \n", argv[0]);
      } else  {
         printf("Usage: %s <filename> \n", argv[0]);
         printf("   <filename>   = filename to verify\n");
      }
   }
   return return_code;
} /* Endbody */

#endif //SHELLCFG_USES_MFS