BEGIN {
   print "-- Generated by mib2def, edit at will"
   mibtype[""]               = 0
   mibtype["INTEGER"]        = 0
   mibtype["Integer32"]      = 0
   mibtype["OCTET"]          = 0
   mibtype["OBJECT"]         = 0
   mibtype["SEQUENCE"]       = 0
   mibtype["NetworkAddress"] = 0
   mibtype["IpAddress"]      = 0
   mibtype["Counter"]        = 0
   mibtype["Counter32"]      = 0
   mibtype["Gauge"]          = 0
   mibtype["Gauge32"]        = 0
   mibtype["Unsigned32"]     = 0
   mibtype["TimeTicks"]      = 0
   mibtype["Opaque"]         = 0
   mibtype["Counter64"]      = 0
   mibtype["BITS"]           = 0
}

($NF == "BEGIN") {state = 1; print "-- Definitions for " $1; next}
($NF == "END")   {state = 0}
(state == 0)     {next}

function nextword() {
   while (i > NF) {
      for (;;) {
         getline
         if (substr($0,1,1) ~ /[ \t]/) break
      }
      i = 1
   }
}

function getimports() {
   j = 0
   for (;; i++) {
      nextword()
      if ($i ~ /;$/) break
      if (j == 1)       {j = 0; continue}
      if ($i == "FROM") {j = 1; continue}
      gsub(/,/,"",$i)
      mibtype[$i] = 0
   }
}

function getmibtype() {
   nextword()
   mibsyntax = $i
}

function getmibparent() {
   j = 0
   for (;; i++) {
      nextword()
      if ($i == "{") {
         continue
      }
      else {
         j++;
      }
      gsub(/}/,"",$i)
      gsub(/{/,"",$i)
      if (j == 1) mibparent = $i
      if (j == 2) mibparent = mibparent "." $i 
      if (j == 3) break
   }
}

function getmibindex() {
   mibindex = ""
   for (;; i++) {
      nextword()
      if ($i == "{") continue
      if ($i == "}") break
      gsub(/,/,"",$i)
      mibindex = mibindex " " $i
   }
}

function gettrapvars() {
   for (;; i++) {
      nextword()
      if ($i == "{") break
   }
   trapvars = ""
   i++
   for (;; i++) {
      nextword()
      if ($i == "}") break
      gsub(/,/,"",$i)
      trapvars = trapvars " " $i
   }
}

function printmib() {
   if (!(mibsyntax in mibtype)) {
      for (;;) {
         getline
         if ($1 " " $2 == mibsyntax " ::=") break
      }
      i = 3
      getmibtype()
   }
   print mibnode,   substr("                                      ",length(mibnode)) \
         mibparent, substr("                                      ",length(mibparent)) \
         mibsyntax, substr("              ",length(mibsyntax)) \
         mibaccess, substr("              ",length(mibaccess)) \
         mibstatus, substr("          ",length(mibstatus)) \
         mibindex
}

function printtrap(v) {
   if (v == 1) {
      if (trapent == "snmp") {
         print "%trap", trapnode, trapval, 0, trapent, trapvars
      } else {
         print "%trap", trapnode, 6, trapval, trapent, trapvars
      }
   } else if (v == 2) {
      print "%trap", trapnode, -1, mibparent, trapvars
   }
}

($1 == "--") {next}

($1 == "IMPORTS") {
   i = 2
   getimports()
}

($2 == "::=") {
   newtype = $1
   i = 3
   getmibtype()
   mibtype[newtype] = 0
   print "%%", newtype, mibsyntax
}

($2 " " $3 " " $4 == "OBJECT IDENTIFIER ::=") {
   mibnode = $1
   mibsyntax = ""
   mibaccess = ""
   mibstatus = ""
   mibindex = ""
   i = 5
   getmibparent()
   printmib()
}

($2 == "OBJECT-TYPE") {
   mibnode = $1
   mibindex = ""
   for (;;) {
      getline
      if ($1 == "SYNTAX") mibsyntax = $2
      if ($1 ~  /ACCESS/) mibaccess = $2
      if ($1 == "STATUS") mibstatus = $2
      if ($1 == "INDEX")  {i = 2; getmibindex()}
      if ($1 == "::=") break
   }
   i = 2
   getmibparent()
   printmib()
}

($2 == "TRAP-TYPE") {
   trapnode = $1
   for (;;) {
      getline
      if ($1 == "ENTERPRISE") trapent = $2
      if ($1 == "VARIABLES")  {i = 2; gettrapvars()}
      if ($1 == "::=")        {i = 2; nextword(); trapval = $i; break}
   }
   printtrap(1)
}

($2 == "NOTIFICATION-TYPE") {
   trapnode = $1
   for (;;) {
      getline
      if ($1 == "OBJECTS")  {i = 2; gettrapvars()}
      if ($1 == "::=")      {i = 2; getmibparent(); break}
   }
   printtrap(2)
}

END {print "%%"}

# EOF
