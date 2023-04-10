/* Test parser for hashtable */
%token IDENT 1 INTLIT 2 ASSIGN 3 SEMI 4 DELETE 5 INSERT 6 LIST 7
%start file
%{
#include <lg.h>
#include <str.h>
#include <hashpp.h>
#include <hshpptem.h>


#define yyerror printf

   class MyHashFunctions : public cHashFunctions
   {
   public:
      static unsigned Hash(const cStr& p)
         {
            return cHashFunctions::Hash((const char*)p);
         }

      static BOOL IsEqual(const cStr& p1, const cStr& p2)
         {
            return cHashFunctions::IsEqual((const char*)(p1),(const char*)(p2));
         }
   };

typedef cHashTable<cStr,int,MyHashFunctions> MyHashTable; 
MyHashTable TestHash(2);

EXTERN yyparse(void);
EXTERN int yylval;
EXTERN yylex(void);
%}
%% 

        file : sequence ;

        sequence : statement SEMI | sequence statement SEMI ;

        statement : _set | _insert  | _delete | _null | _lookup | _list ;

        _null : ; 

        _set : IDENT ASSIGN INTLIT 
               {
                  cStr key((char*)$1);
                  int val = (int)$3;
                  TestHash.Set(key,val);
               } ; 

        _insert : INSERT IDENT ASSIGN INTLIT 
               {
                  cStr key((char*)$2);
                  int val = (int)$4;
                  TestHash.Insert(key,val);
               } ; 

        _delete : DELETE IDENT 
               {
                  cStr key((char*)$2);
                  TestHash.Delete(key);
               } ; 

        _lookup : IDENT
               {
                  cStr key((char*)$1);
                  int result;
                  if (TestHash.Lookup(key,result))
                     printf("%s: %d\n",(const char*)key,result);
                  else
                     printf("%s not fount\n",(const char*)key);

               }  ;

        _list   : LIST 
              {
                 MyHashTable::cIter iter = TestHash.Iter();
                 for (;!iter.Done(); iter.Next())
                 {
                    printf("%s: %d\n",(const char*)iter.Key(),iter.Value());
                 }
              } ;


%% 


main()
{
  DbgMonoConfig();
  while(TRUE) yyparse();
}

