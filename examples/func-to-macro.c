#define better_quad(a, b, c, x) (a*x*x + b*x + c)

/* Calculate the result of a quadratic equation */
float quad(float a, float b, float c, float x)
{
	return (a*(x*x)) + b*x + c;
}

/* 
 * The intent of this file is to observe the outcome after preprocessing via
 * something like `watch -d 'cat func-to-macro.post-cpp'` so to simplify the
 * outcome as much as possible, variables and names are intentionally kept terse
 */
int main(int args, char *argv[])
{
	float x = 0;
	float a = 1.0;
	float b = 2.0;
	float c = 3.0;
	int result = 0;

	/* Function call */
	for (x = 0; x < 1; x = x + 0.1) {
		result = (int) quad(a, b, c, x);
	}
	/* Macro call */
	for (x = 0; x < 1; x = x + 0.1) {
		result = (int) better_quad(a, b, c, x);
	}

	return result;
}

