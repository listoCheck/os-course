import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function unlink(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const ino = parseInt(ctx.query.parent_ino as string, 10) || 0;
    const success = repository.delete(ino, token);
    ctx.body = {success}
    await next();
}