/****************************************************************************
   SCOORE: Santa Cruz Out-of-order Risk Engine
   Copyright (C) 2004, 2006 University of California, Santa Cruz.


This file is part of SCOORE.

SCOORE is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2, or (at your option) any later version.

SCOORE is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SCOORE; see the file COPYING.  If not, write to the  Free Software Foundation,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
WITH THE SOFTWARE.

****************************************************************************/

/****************************************************************************
    BUGS Found and/or Corrected:

****************************************************************************/

/****************************************************************************
    Description:

****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

main(int argc, char **argv) {

  if (argc <= 1) {
    fprintf(stderr,"usage:\n\t%s <mantisa>\n",argv[0]);
    exit(-1);
  }

  if (argv[1][1] == 'x') {
    unsigned long long val = strtoll(argv[1],0, 16);

    double v1 = *(double *)&val;
    val = val>>12;
    double v2 = *(double *)&val;
    
    printf("v1[%.56f] v2[%.56f] (long long)\n",v1, v2);
  }else{
    double val = atof(argv[1]);

    unsigned long long v1 = *(unsigned long long *)&val;
    unsigned long long v2 = v1>>12;
   
    printf("v1[0x%lld] v2[0x%lld] (double)\n",v1, v2);
  }
}
