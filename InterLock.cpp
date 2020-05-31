#include <Windows.h>
#include <iostream>
using namespace std;


#define IN_VALUE	0x01234567
void main()
{
	LONG target = 12;
	LONG orgVal, retVal;

	target = orgVal = 12;
	retVal = InterlockedExchange(&target, 5);
	printf("InterlockedExchange => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);
	/*
	target 변수를 value의 값으로 대체한다. 결국 target에 value를 대입하는 것과 동일하다.
	리턴값은 target에 value를 대입하기 전의 원래 값으로 돌려준다.
	*/
	target = orgVal = 125;
	retVal = InterlockedCompareExchange(&target, 384, 125);
	printf("InterlockedCompareExchange cmp 125 => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);

	target = orgVal = 8;
	retVal = InterlockedCompareExchange(&target, 384, 125);
	printf("InterlockedCompareExchange cmp 125 => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);
	/*
	호출 시에 사용자를 상당히 혼란스럽게 만드는 함수다.
	target이 cmp와 같으면 target을 value로 대체하고, 다르면 target을 변경하지 않는다.
	리턴값은 target이 변경되든 그렇지 않든 원래 값으로 돌려준다.
	*/
	target = orgVal = 12;
	retVal = InterlockedExchangeAdd(&target, 5);
	printf("InterlockedExchangeAdd with 5 => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);

	target = orgVal = 12;
	retVal = InterlockedExchangeAdd(&target, -25);
	printf("InterlockedExchangeAdd with -25 => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);
	//target에 value를 더한다.빼고자 한다면 value를 음수로 설정하여 호출하면 된다.
	//리턴값은target에 value가 더해지기 전의 값이다.

	target = orgVal = 12;
	retVal = InterlockedIncrement(&target);
	printf("InterlockedIncrement => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);
	target = orgVal = 12;
	retVal = InterlockedDecrement(&target);
	printf("InterlockedDecrement => "
		"orgVal=%d, retVal=%d, target=%d\n", orgVal, retVal, target);
	/*
	target을 1 증가하거나 감소한다. 이 함수의 리턴값은  target의 증감 결과를 돌려준다.
	즉 target++ 또는 target--의 결괏값이 리턴되는데,
	결국 호출 후의 리턴값과 target 값이 같아진다는 점에 주의하기 바란다.
	*/
	target = orgVal = 0x01234567;
	retVal = InterlockedAnd(&target, 0xFF);
	printf("InterlockedAnd => "
		"orgVal=0x%08x, retVal=0x%08x, target=0x%08x\n", orgVal, retVal, target);
	target = orgVal = 0x01234567;
	retVal = InterlockedOr(&target, 0xFF);
	printf("InterlockedOr => "
		"orgVal=0x%08x, retVal=0x%08x, target=0x%08x\n", orgVal, retVal, target);
	target = orgVal = 0x01234567;
	retVal = InterlockedXor(&target, 0xFF);
	printf("InterlockedXor => "
		"orgVal=0x%08x, retVal=0x%08x, target=0x%08x\n", orgVal, retVal, target);
	/*
	target에 value를 각각 AND, OR, XOR 연산을 수행한 후 그 결괏값을 target 값으로 설정한다.
	리턴값은 변경되기 전의 원래 target 값이다.
	*/

	char bitBuff[33] = { 0, };

	target = orgVal = 0x01234567;
	for (int i = 31; i >= 0; i--)
	{
		BOOL val = InterlockedBitTestAndSet(&target, i);
		bitBuff[31 - i] = (val ? '1' : '0');
	}
	printf("InterlockedBitTestAndSet => %s\n\t"
		"orgVal=0x%08x, retVal=0x%08x, target=0x%08x\n", bitBuff, orgVal, retVal, target);

	target = orgVal = 0x01234567;
	for (int i = 31; i >= 0; i--)
	{
		BOOL val = InterlockedBitTestAndReset(&target, i);
		bitBuff[31 - i] = (val ? '1' : '0');
	}
	printf("InterlockedBitTestAndReset => %s\n\t"
		"orgVal=0x%08x, retVal=0x%08x, target=0x%08x\n", bitBuff, orgVal, retVal, target);

	/*
	target의 bitPos 위치의 비트값을 리턴값으로 돌려준다. 즉 0이면 FALSE, 1이면 TRUE다.
	동시에 Set 함수의 경우는 target의 bitPos 위치의 비트값이 0이면 1로 설정하고,
	Reset 함수의 경우는 target의 bitPos 위치의 비트값이 1이면 0으로 설정한다.
	*/
}
