import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function unlink(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const ino = parseInt(ctx.query.parent_ino as string, 10) || 0;

    console.log("[unlink] Incoming request:", { token, ino });

    const success = await repository.delete(ino, token);

    console.log("[unlink] Response:", { success });

    ctx.body = { success };
    await next();
}