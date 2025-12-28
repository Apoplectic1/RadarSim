from mcp.server.fastmcp import FastMCP
from google import genai
import os

mcp = FastMCP("gemini")

# UPDATED LINE: Use the specific "002" stable version
@mcp.tool()
def ask_gemini(prompt: str, model: str = "gemini-1.5-flash-002") -> str:
    """
    Ask Gemini to analyze code.
    Args:
        prompt: The text to analyze.
        model: "gemini-1.5-flash-002" (stable) or "gemini-2.0-flash-exp" (experimental).
    """
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        return "Error: GEMINI_API_KEY not found."

    try:
        client = genai.Client(api_key=api_key)
        response = client.models.generate_content(
            model=model,
            contents=prompt
        )
        return response.text
    except Exception as e:
        return f"Gemini API Error: {str(e)}"

if __name__ == "__main__":
    mcp.run(transport='stdio')
